#include "task.h"
#include "sched.h"
#include "timer.h"
#include "kernel.h"
#include "miniprintf.h"
#include "xtensa_ops.h"

/* Forward declaration from vectors.S */
extern void _context_switch(uint32_t **old_sp, uint32_t *new_sp);
extern void _task_entry(void);

/* Global: current task's saved stack pointer (used by ISR) */
uint32_t *_current_task_sp;

/* Task table */
tcb_t tcb_table[MAX_TASKS];
int num_tasks = 0;

/* Currently running task */
static tcb_t *current_task = (tcb_t *)0;

/* Need access to timer ticks in sched */
uint32_t timer_get_ticks_internal(void)
{
    extern volatile uint32_t system_ticks;
    return system_ticks;
}

void task_init(void)
{
    num_tasks = 0;
    current_task = (tcb_t *)0;
    _current_task_sp = (uint32_t *)0;

    for (int i = 0; i < MAX_TASKS; i++) {
        tcb_table[i].state = TASK_UNUSED;
        tcb_table[i].pid = i;
        tcb_table[i].sp = (uint32_t *)0;
        tcb_table[i].stack_base = (uint32_t *)0;
        tcb_table[i].stack_size = 0;
        tcb_table[i].next = (tcb_t *)0;
    }

    sched_init();
}

int task_create(task_func_t func, void *arg, uint32_t stack_size, int priority)
{
    /* Find free TCB */
    int pid = -1;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (tcb_table[i].state == TASK_UNUSED) {
            pid = i;
            break;
        }
    }
    if (pid < 0)
        return -1;

    tcb_t *tcb = &tcb_table[pid];

    /* Allocate stack (aligned to 16 bytes) */
    uint32_t stack_words = (stack_size + 3) / 4;
    uint32_t *stack = (uint32_t *)kmalloc(stack_words * 4);
    if (!stack)
        return -1;

    tcb->stack_base = stack;
    tcb->stack_size = stack_words * 4;
    tcb->priority = priority;
    tcb->state = TASK_READY;
    tcb->wake_tick = 0;
    tcb->next = (tcb_t *)0;

    /*
     * Initialize stack for call0 ABI context switch.
     * Stack layout (grows downward):
     *
     *   High address (stack_base + stack_size):
     *     [ arg ]        ← a2 = task argument
     *     [ func ]       ← initial value to be loaded into a2 for _task_entry
     *     [ 0 ]          ← padding
     *     [ _task_entry ] ← a0 = return address (where context_switch "returns" to)
     *
     *   Context save area (popped by _context_switch):
     *     [ SAR ]
     *     [ a15 ]
     *     [ a14 ]
     *     [ a13 ]
     *     [ a12 ]
     *     [ a0 = _task_entry ]
     *
     *   Low address (sp points here):
     */
    uint32_t *sp = stack + stack_words;

    /* Set up initial stack frame for _task_entry */
    sp -= 1;
    *sp = (uint32_t)arg;           /* Argument for the task function */

    sp -= 1;
    *sp = (uint32_t)func;          /* Task function pointer */

    sp -= 1;
    *sp = 0;                       /* Padding */

    sp -= 1;
    *sp = (uint32_t)_task_entry;   /* Return address for context_switch */

    /* Context save area that _context_switch expects */
    sp -= 6;  /* a0, a12, a13, a14, a15, SAR */
    sp[0] = (uint32_t)_task_entry; /* a0 */
    sp[1] = 0;                     /* a12 */
    sp[2] = 0;                     /* a13 */
    sp[3] = 0;                     /* a14 */
    sp[4] = 0;                     /* a15 */
    sp[5] = 0;                     /* SAR */

    tcb->sp = sp;

    /* Add to scheduler ready queue */
    sched_add(tcb);

    num_tasks++;
    return pid;
}

tcb_t *task_current(void)
{
    return current_task;
}

void task_yield(void)
{
    if (!current_task)
        return;

    uint32_t state = irq_save();

    tcb_t *prev = current_task;
    sched_add(prev);

    tcb_t *next = sched_pick_next();
    if (!next) {
        irq_restore(state);
        return;
    }

    current_task = next;
    next->state = TASK_RUNNING;
    _current_task_sp = next->sp;

    /* Context switch: save prev->sp, load next->sp */
    _context_switch(&prev->sp, next->sp);

    irq_restore(state);
}

void task_block(void)
{
    if (!current_task)
        return;

    uint32_t state = irq_save();

    current_task->state = TASK_BLOCKED;

    tcb_t *next = sched_pick_next();
    if (!next) {
        /* No task to run - this shouldn't happen with idle task */
        irq_restore(state);
        return;
    }

    tcb_t *prev = current_task;
    current_task = next;
    next->state = TASK_RUNNING;
    _current_task_sp = next->sp;

    _context_switch(&prev->sp, next->sp);

    irq_restore(state);
}

void task_unblock(tcb_t *task)
{
    uint32_t state = irq_save();
    if (task->state == TASK_BLOCKED) {
        sched_add(task);
    }
    irq_restore(state);
}

void task_sleep_ms(uint32_t ms)
{
    if (!current_task)
        return;

    current_task->wake_tick = timer_get_ms() + ms;
    task_block();
}

void task_exit(void)
{
    if (!current_task)
        return;

    uint32_t state = irq_save();

    current_task->state = TASK_DELETED;
    num_tasks--;

    /* Free stack */
    if (current_task->stack_base) {
        kfree(current_task->stack_base);
        current_task->stack_base = (uint32_t *)0;
    }

    /* Pick next task */
    tcb_t *next = sched_pick_next();
    if (next) {
        tcb_t *prev = current_task;
        current_task = next;
        next->state = TASK_RUNNING;
        _current_task_sp = next->sp;

        /* Don't save prev context since task is dead */
        _context_switch((uint32_t **)0, next->sp);
    }

    irq_restore(state);
}

/*
 * Called by scheduler to switch to a specific task.
 * Used by timer ISR for preemptive switching.
 */
void task_switch_to(tcb_t *next)
{
    if (!next || next == current_task)
        return;

    tcb_t *prev = current_task;
    current_task = next;
    next->state = TASK_RUNNING;

    if (prev && prev->state == TASK_RUNNING) {
        prev->state = TASK_READY;
    }

    _current_task_sp = next->sp;

    if (prev) {
        _context_switch(&prev->sp, next->sp);
    } else {
        /* First task - just load its context */
        /* This is handled differently - we set sp and jump */
    }
}

/*
 * Create idle task (lowest priority, never blocks)
 */
static void idle_task_func(void *arg)
{
    (void)arg;
    while (1) {
        /* Do nothing, just yield */
        task_yield();
    }
}

int task_create_idle(void)
{
    return task_create(idle_task_func, (void *)0, 512, TASK_PRIORITY_IDLE);
}
