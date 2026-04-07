#ifndef IRQ_H
#define IRQ_H

#include "types.h"

/* Maximum CPU interrupt handlers */
#define MAX_CPU_INTERRUPTS  32

/* Interrupt handler function type */
typedef void (*irq_handler_t)(void);

/* Initialize interrupt controller */
void irq_init(void);

/* Register a handler for a CPU interrupt number */
void irq_register_handler(int cpu_int_num, irq_handler_t handler);

/* Map a peripheral interrupt source to a CPU interrupt */
void irq_map_peripheral(int cpu_int_num, int periph_source);

/* Set CPU interrupt priority (1-7) */
void irq_set_priority(int cpu_int_num, int priority);

/* Enable/disable a specific CPU interrupt */
void irq_enable_int(int cpu_int_num);
void irq_disable_int(int cpu_int_num);

/* Clear a pending CPU interrupt */
void irq_clear_int(int cpu_int_num);

/* Global interrupt enable/disable */
uint32_t irq_save(void);
void irq_restore(uint32_t state);

#endif /* IRQ_H */
