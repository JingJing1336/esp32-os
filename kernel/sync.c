#include "sync.h"
#include "task.h"
#include "xtensa_ops.h"

/* ============================================================
 * Mutex Implementation
 * ============================================================ */

void mutex_init(mutex_t *m)
{
    m->owner = (tcb_t *)0;
    m->lock_count = 0;
    m->wait_head = (tcb_t *)0;
    m->wait_tail = (tcb_t *)0;
}

void mutex_lock(mutex_t *m)
{
    uint32_t state = irq_save();

    tcb_t *current = task_current();

    if (!m->owner) {
        /* Mutex is free - acquire it */
        m->owner = current;
        m->lock_count = 1;
        irq_restore(state);
        return;
    }

    /* Mutex is held - block current task */
    current->state = TASK_BLOCKED;

    /* Add to wait queue */
    current->next = (tcb_t *)0;
    if (!m->wait_tail) {
        m->wait_head = current;
        m->wait_tail = current;
    } else {
        m->wait_tail->next = current;
        m->wait_tail = current;
    }

    irq_restore(state);

    /* Yield CPU */
    task_block();
}

void mutex_unlock(mutex_t *m)
{
    uint32_t state = irq_save();

    tcb_t *current = task_current();

    if (m->owner != current) {
        irq_restore(state);
        return;
    }

    m->lock_count--;
    if (m->lock_count > 0) {
        irq_restore(state);
        return;
    }

    /* Release the mutex */
    m->owner = (tcb_t *)0;

    /* Wake up first waiting task */
    if (m->wait_head) {
        tcb_t *waiter = m->wait_head;
        m->wait_head = waiter->next;
        if (!m->wait_head)
            m->wait_tail = (tcb_t *)0;

        waiter->next = (tcb_t *)0;

        /* Give mutex to waiter */
        m->owner = waiter;
        m->lock_count = 1;

        /* Unblock the waiter */
        task_unblock(waiter);
    }

    irq_restore(state);
}

int mutex_trylock(mutex_t *m)
{
    uint32_t state = irq_save();

    if (!m->owner) {
        m->owner = task_current();
        m->lock_count = 1;
        irq_restore(state);
        return 1;  /* Success */
    }

    irq_restore(state);
    return 0;  /* Failed */
}

/* ============================================================
 * Counting Semaphore Implementation
 * ============================================================ */

void sem_init(semaphore_t *s, int initial, int max)
{
    s->count = initial;
    s->max_count = max;
    s->wait_head = (tcb_t *)0;
    s->wait_tail = (tcb_t *)0;
}

void sem_wait(semaphore_t *s)
{
    uint32_t state = irq_save();

    if (s->count > 0) {
        s->count--;
        irq_restore(state);
        return;
    }

    /* Count is 0 - block current task */
    tcb_t *current = task_current();
    current->state = TASK_BLOCKED;

    /* Add to wait queue */
    current->next = (tcb_t *)0;
    if (!s->wait_tail) {
        s->wait_head = current;
        s->wait_tail = current;
    } else {
        s->wait_tail->next = current;
        s->wait_tail = current;
    }

    irq_restore(state);
    task_block();
}

void sem_signal(semaphore_t *s)
{
    uint32_t state = irq_save();

    if (s->wait_head) {
        /* Wake up first waiting task */
        tcb_t *waiter = s->wait_head;
        s->wait_head = waiter->next;
        if (!s->wait_head)
            s->wait_tail = (tcb_t *)0;

        waiter->next = (tcb_t *)0;
        task_unblock(waiter);
    } else {
        /* No waiters - increment count */
        if (s->count < s->max_count)
            s->count++;
    }

    irq_restore(state);
}

int sem_trywait(semaphore_t *s)
{
    uint32_t state = irq_save();

    if (s->count > 0) {
        s->count--;
        irq_restore(state);
        return 1;  /* Success */
    }

    irq_restore(state);
    return 0;  /* Failed */
}
