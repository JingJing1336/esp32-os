#include "timer.h"
#include "irq.h"
#include "esp32_regs.h"
#include "xtensa_ops.h"

/* Global tick counter */
static volatile uint32_t system_ticks = 0;

/*
 * Timer Group 0 Timer 0 ISR
 * Called every 1ms (TICK_RATE_HZ = 1000).
 */
static void timer_isr(void)
{
    /* Clear timer interrupt */
    TG0_INT_CLR_REG = TG0_T0_INT_CLR;

    /* Increment system tick counter */
    system_ticks++;

    /* Update alarm for next tick */
    uint32_t lo = TG0_T0ALARMLO_REG;
    uint32_t hi = TG0_T0ALARMHI_REG;

    /* Add tick interval to alarm */
    uint32_t divider = APB_CLK_FREQ / TICK_RATE_HZ;  /* 80000 */
    lo += divider;
    if (lo < divider)  /* overflow */
        hi++;

    TG0_T0ALARMLO_REG = lo;
    TG0_T0ALARMHI_REG = hi;

    /* Re-enable alarm */
    TG0_T0CONFIG_REG |= TIMER_ALARM_EN;
}

void timer_init(void)
{
    /* Enable Timer Group 0 clock */
    DPORT_PERI_CLK_EN_REG |= DPORT_TIMG0_CLK_EN;

    /* Reset Timer Group 0 */
    DPORT_PERI_RST_EN_REG &= ~DPORT_TIMG0_RST;

    /* Disable timer while configuring */
    TG0_T0CONFIG_REG = 0;

    /* Set divider: APB_CLK (80MHz) / 1 = 80MHz timer clock */
    /* Actually use divider for 80kHz → 80MHz/1000 = 80kHz */
    /* divider field is 16 bits at bit 13, actual divider = value * 2 */
    /* For simplicity: divider = 1 means timer runs at APB_CLK / 2 = 40MHz */
    /* Let's use divider = 2 → APB_CLK / (2*2) = 20MHz */
    /* Actually, divider is in CONFIG register bits [28:13] */
    /* divider = 16-bit value, actual prescale = reg_val */
    uint32_t config = 0;
    config |= (1 << TIMER_DIVIDER_SHIFT);   /* divider = 1 → prescale by 1 */
    config |= TIMER_INCREASE;               /* count up */
    config |= TIMER_LEVEL_INT;              /* level interrupt */

    TG0_T0CONFIG_REG = config;

    /* Clear timer counter */
    TG0_T0LOADLO_REG = 0;
    TG0_T0LOADHI_REG = 0;
    TG0_T0LOAD_REG = 0;  /* trigger load */

    /* Set alarm value: APB_CLK / TICK_RATE_HZ = 80,000,000 / 1000 = 80000 */
    uint32_t alarm_val = APB_CLK_FREQ / TICK_RATE_HZ;
    TG0_T0ALARMLO_REG = alarm_val;
    TG0_T0ALARMHI_REG = 0;

    /* Enable timer alarm */
    TG0_T0CONFIG_REG |= TIMER_ALARM_EN | TIMER_AUTORELOAD;

    /* Map Timer Group 0 Timer 0 interrupt to CPU interrupt */
    irq_map_peripheral(CPU_INTC_TIMER, ETS_TG0_T0_LEVEL_INTR_SOURCE);
    irq_set_priority(CPU_INTC_TIMER, 1);  /* Priority 1 */
    irq_register_handler(CPU_INTC_TIMER, timer_isr);
    irq_enable_int(CPU_INTC_TIMER);

    /* Enable timer interrupt in timer group */
    TG0_INT_ENA_REG |= TG0_T0_INT_CLR;  /* Enable TG0 timer0 interrupt */

    /* Start the timer */
    TG0_T0CONFIG_REG |= TIMER_EN;
}

uint32_t timer_get_ticks(void)
{
    return system_ticks;
}

uint32_t timer_get_ms(void)
{
    return system_ticks;
}

void timer_delay_ms(uint32_t ms)
{
    uint32_t start = system_ticks;
    while ((system_ticks - start) < ms)
        ;
}
