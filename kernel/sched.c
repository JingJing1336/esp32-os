#include "sched.h"
#include "kernel.h"
#include "miniprintf.h"

/* Ready queues: one per priority level (simplified, using linked lists) */
#define NUM_PRIORITY_LEVELS 32

static tcb_t *ready_head[NUM_PRIORITY_LEVELS];
static tcb_t *ready_tail[NUM_PRIORITY_LEVELS];

void sched_init(void)
{
    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        ready_head[i] = (tcb_t *)0;
        ready_tail[i] = (tcb_t *)0;
    }
}

void sched_add(tcb_t *task)
{
    int pri = task->priority;
    task->next = (tcb_t *)0;
    task->state = TASK_READY;

    if (!ready_tail[pri]) {
        ready_head[pri] = task;
        ready_tail[pri] = task;
    } else {
        ready_tail[pri]->next = task;
        ready_tail[pri] = task;
    }
}

void sched_remove(tcb_t *task)
{
    int pri = task->priority;
    tcb_t *prev = (tcb_t *)0;
    tcb_t *curr = ready_head[pri];

    while (curr) {
        if (curr == task) {
            if (prev)
                prev->next = curr->next;
            else
                ready_head[pri] = curr->next;

            if (curr == ready_tail[pri])
                ready_tail[pri] = prev;

            task->next = (tcb_t *)0;
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

tcb_t *sched_pick_next(void)
{
    /* Scan from highest priority (0) to lowest */
    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        if (ready_head[i]) {
            tcb_t *task = ready_head[i];

            /* Round-robin: move this task to the end of its queue */
            ready_head[i] = task->next;
            if (!ready_head[i]) {
                ready_tail[i] = (tcb_t *)0;
            } else {
                task->next = (tcb_t *)0;
                ready_tail[i]->next = task;
                ready_tail[i] = task;
            }

            return task;
        }
    }
    return (tcb_t *)0;
}

void sched_tick(void)
{
    /* Check for tasks that need to wake up from sleep */
    extern tcb_t tcb_table[];
    extern int num_tasks;
    uint32_t now = timer_get_ticks_internal();

    for (int i = 0; i < num_tasks; i++) {
        if (tcb_table[i].state == TASK_BLOCKED &&
            tcb_table[i].wake_tick > 0 &&
            tcb_table[i].wake_tick <= now) {
            tcb_table[i].wake_tick = 0;
            sched_add(&tcb_table[i]);
        }
    }
}
