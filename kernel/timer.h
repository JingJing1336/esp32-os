#ifndef TIMER_H
#define TIMER_H

#include "types.h"

/* System tick frequency */
#define TICK_RATE_HZ    1000    /* 1ms per tick */
#define TICK_MS         1

/* Initialize system timer */
void timer_init(void);

/* Get current tick count */
uint32_t timer_get_ticks(void);

/* Get milliseconds since boot */
uint32_t timer_get_ms(void);

/* Busy-wait delay in milliseconds */
void timer_delay_ms(uint32_t ms);

#endif /* TIMER_H */
