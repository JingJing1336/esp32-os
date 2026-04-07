#include "kernel.h"
#include "task.h"
#include "sync.h"
#include "timer.h"
#include "miniprintf.h"

/* Shared data for producer-consumer demo */
static volatile int shared_counter = 0;
static mutex_t     data_mutex;
static semaphore_t data_sem;

/*
 * Task 1: Status reporter
 * Prints system status every 2 seconds.
 */
static void task_status(void *arg)
{
    (void)arg;
    int count = 0;

    while (1) {
        mutex_lock(&data_mutex);
        int val = shared_counter;
        mutex_unlock(&data_mutex);

        miniprintf("[status] tick=%d  counter=%d  loop=%d\r\n",
                   timer_get_ms(), val, count);
        count++;
        task_sleep_ms(2000);
    }
}

/*
 * Task 2: Producer
 * Increments shared counter every 500ms,
 * signals the consumer via semaphore.
 */
static void task_producer(void *arg)
{
    (void)arg;

    while (1) {
        mutex_lock(&data_mutex);
        shared_counter++;
        miniprintf("[producer] produced: %d\r\n", shared_counter);
        mutex_unlock(&data_mutex);

        sem_signal(&data_sem);
        task_sleep_ms(500);
    }
}

/*
 * Task 3: Consumer
 * Waits on semaphore, then reads and displays the counter.
 */
static void task_consumer(void *arg)
{
    (void)arg;

    while (1) {
        sem_wait(&data_sem);

        mutex_lock(&data_mutex);
        int val = shared_counter;
        mutex_unlock(&data_mutex);

        miniprintf("[consumer] consumed: %d\r\n", val);
    }
}

/*
 * Application entry point - called by kernel_main()
 */
void app_main(void)
{
    miniprintf("[app] Initializing synchronization primitives...\r\n");

    mutex_init(&data_mutex);
    sem_init(&data_sem, 0, 10);

    miniprintf("[app] Creating demo tasks...\r\n");

    int pid1 = task_create(task_status,   (void *)0, 1024, TASK_PRIORITY_LOW);
    int pid2 = task_create(task_producer, (void *)0, 1024, TASK_PRIORITY_NORMAL);
    int pid3 = task_create(task_consumer, (void *)0, 1024, TASK_PRIORITY_NORMAL);

    miniprintf("[app] Tasks created: status=%d, producer=%d, consumer=%d\r\n",
               pid1, pid2, pid3);
}
