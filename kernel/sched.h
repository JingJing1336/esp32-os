#ifndef SCHED_H
#define SCHED_H

#include "task.h"

/* Initialize scheduler */
void sched_init(void);

/* Add task to ready queue */
void sched_add(tcb_t *task);

/* Remove task from ready queue */
void sched_remove(tcb_t *task);

/* Pick the next task to run (highest priority, then round-robin) */
tcb_t *sched_pick_next(void);

/* Scheduler tick - called from timer ISR */
void sched_tick(void);

#endif /* SCHED_H */
