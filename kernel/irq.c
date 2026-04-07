#include "irq.h"
#include "esp32_regs.h"
#include "xtensa_ops.h"

/* Handler table for CPU interrupts */
static irq_handler_t irq_handlers[MAX_CPU_INTERRUPTS];

void irq_init(void)
{
    /* Disable all CPU interrupts */
    INTPRI_CORE0_CPU_INT_ENABLE_REG = 0;

    /* Clear all interrupt types and priorities */
    for (int i = 0; i < 4; i++) {
        INTPRI_CORE0_CPU_INT_TYPE_REG(i) = 0;
        INTPRI_CORE0_CPU_INT_PRI_REG(i) = 0;
    }

    /* Set interrupt threshold to 1 (allow level 1+ interrupts) */
    INTPRI_CORE0_CPU_INT_THRESH_REG = 1;

    /* Clear all handler pointers */
    for (int i = 0; i < MAX_CPU_INTERRUPTS; i++) {
        irq_handlers[i] = (irq_handler_t)0;
    }

    /* Disable all peripheral interrupt sources in INTC */
    for (int i = 0; i < 64; i++) {
        /* Map all to CPU interrupt 0 (disabled) */
        INTPRI_CORE0_INT_MAP_REG(i) = 0;
    }
}

void irq_register_handler(int cpu_int_num, irq_handler_t handler)
{
    if (cpu_int_num >= 1 && cpu_int_num < MAX_CPU_INTERRUPTS) {
        irq_handlers[cpu_int_num] = handler;
    }
}

void irq_map_peripheral(int cpu_int_num, int periph_source)
{
    if (periph_source >= 0 && periph_source < 64) {
        INTPRI_CORE0_INT_MAP_REG(periph_source) = cpu_int_num;
    }
}

void irq_set_priority(int cpu_int_num, int priority)
{
    /* Each priority register holds 8 entries */
    int reg = cpu_int_num / 8;
    int shift = (cpu_int_num % 8) * 4;

    uint32_t val = INTPRI_CORE0_CPU_INT_PRI_REG(reg);
    val &= ~(0xF << shift);
    val |= ((priority & 0xF) << shift);
    INTPRI_CORE0_CPU_INT_PRI_REG(reg) = val;
}

void irq_enable_int(int cpu_int_num)
{
    INTPRI_CORE0_CPU_INT_ENABLE_REG |= (1U << cpu_int_num);
}

void irq_disable_int(int cpu_int_num)
{
    INTPRI_CORE0_CPU_INT_ENABLE_REG &= ~(1U << cpu_int_num);
}

void irq_clear_int(int cpu_int_num)
{
    INTPRI_CORE0_CPU_INT_EOI_REG(cpu_int_num / 32) = (1U << (cpu_int_num % 32));
}

/*
 * Called from the Xtensa interrupt exception handler.
 * Dispatches to the registered handler based on the pending interrupt.
 */
void irq_dispatch(void)
{
    uint32_t pending = xtensa_get_interrupt();
    pending &= xtensa_get_intenable();

    /* Find the highest priority pending interrupt */
    for (int i = 0; i < MAX_CPU_INTERRUPTS && pending; i++) {
        if (pending & (1U << i)) {
            if (irq_handlers[i]) {
                irq_handlers[i]();
            }
            irq_clear_int(i);
            pending &= ~(1U << i);
        }
    }
}

uint32_t irq_save(void)
{
    return irq_disable_save();
}

void irq_restore(uint32_t state)
{
    irq_restore(state);
}
