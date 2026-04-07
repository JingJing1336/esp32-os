#ifndef SYNC_H
#define SYNC_H

#include "types.h"
#include "task.h"

/* ============================================================
 * Mutex
 * ============================================================ */
typedef struct {
    tcb_t      *owner;      /* Current holder (NULL if unlocked) */
    int         lock_count;  /* Recursive lock count */
    tcb_t      *wait_head;  /* First task waiting for mutex */
    tcb_t      *wait_tail;  /* Last task waiting for mutex */
} mutex_t;

void mutex_init(mutex_t *m);
void mutex_lock(mutex_t *m);
void mutex_unlock(mutex_t *m);
int  mutex_trylock(mutex_t *m);

/* ============================================================
 * Counting Semaphore
 * ============================================================ */
typedef struct {
    int         count;       /* Current count */
    int         max_count;   /* Maximum count */
    tcb_t      *wait_head;  /* First task waiting */
    tcb_t      *wait_tail;  /* Last task waiting */
} semaphore_t;

void sem_init(semaphore_t *s, int initial, int max);
void sem_wait(semaphore_t *s);
void sem_signal(semaphore_t *s);
int  sem_trywait(semaphore_t *s);

#endif /* SYNC_H */
