#ifndef TASK_H
#define TASK_H

#include "types.h"

/* Task states */
typedef enum {
    TASK_UNUSED = 0,
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_DELETED
} task_state_t;

/* Task priorities (0 = highest) */
#define TASK_PRIORITY_IDLE      31
#define TASK_PRIORITY_LOW       16
#define TASK_PRIORITY_NORMAL    8
#define TASK_PRIORITY_HIGH      4
#define TASK_PRIORITY_MAX       0

/* Maximum number of tasks */
#define MAX_TASKS   16

/* Task Control Block */
typedef struct tcb {
    uint32_t       *sp;          /* Saved stack pointer */
    uint32_t       *stack_base;  /* Stack base address */
    uint32_t        stack_size;  /* Stack size in bytes */
    int             pid;         /* Task ID */
    int             priority;    /* Current priority */
    task_state_t    state;       /* Task state */
    uint32_t        wake_tick;   /* Tick to wake up (for sleep) */
    struct tcb     *next;        /* Linked list pointer (for queues) */
} tcb_t;

/* Task function type */
typedef void (*task_func_t)(void *arg);

/* Initialize task subsystem */
void task_init(void);

/* Create a new task */
int task_create(task_func_t func, void *arg, uint32_t stack_size, int priority);

/* Get current task's TCB */
tcb_t *task_current(void);

/* Yield CPU to next ready task */
void task_yield(void);

/* Block current task (change state and yield) */
void task_block(void);

/* Unblock a task (make it ready) */
void task_unblock(tcb_t *task);

/* Sleep for specified milliseconds */
void task_sleep_ms(uint32_t ms);

/* Called when a task function returns */
void task_exit(void);

/* Current task SP (used by assembly context switch) */
extern uint32_t *_current_task_sp;

#endif /* TASK_H */
