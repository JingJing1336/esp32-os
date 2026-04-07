#include "kernel.h"
#include "uart.h"
#include "gpio.h"
#include "miniprintf.h"
#include "irq.h"
#include "timer.h"
#include "task.h"
#include "memory.h"
#include "sched.h"
#include "xtensa_ops.h"

/* Linker-defined symbols */
extern uint32_t _bss_start;
extern uint32_t _bss_end;
extern uint32_t _heap_start;
extern uint32_t _heap_end;
extern uint32_t _stack_top;

/* Forward declaration of user application */
extern void app_main(void);

void kernel_main(void)
{
    /* BSS already cleared by start.S, but verify */
    uart_init();

    uart_puts("\r\n");
    uart_puts("========================================\r\n");
    uart_puts("  MiniKernel v0.1 for ESP-WROOM-32\r\n");
    uart_puts("  Xtensa LX6 - Bare Metal\r\n");
    uart_puts("========================================\r\n");

    /* Print memory layout */
    miniprintf("[kernel] IRAM code section loaded\r\n");
    miniprintf("[kernel] Stack top: 0x%x\r\n", (uint32_t)&_stack_top);
    miniprintf("[kernel] Heap: 0x%x - 0x%x\r\n",
               (uint32_t)&_heap_start, (uint32_t)&_heap_end);

    /* Initialize heap */
    uint32_t heap_size = (uint32_t)&_heap_end - (uint32_t)&_heap_start;
    kheap_init((void *)&_heap_start, heap_size);
    miniprintf("[kernel] Heap initialized: %d bytes\r\n", heap_size);

    /* Initialize interrupt controller */
    irq_init();
    miniprintf("[kernel] Interrupt controller initialized\r\n");

    /* Initialize task subsystem */
    task_init();
    miniprintf("[kernel] Task subsystem initialized\r\n");

    /* Initialize system timer */
    timer_init();
    miniprintf("[kernel] System timer started (1000 Hz tick)\r\n");

    /* Create idle task */
    int idle_pid = task_create_idle();
    miniprintf("[kernel] Idle task created (pid=%d)\r\n", idle_pid);

    /* Run user application setup */
    miniprintf("[kernel] Starting application...\r\n\r\n");
    app_main();

    /* Enable interrupts */
    irq_global_enable();

    /* Start the first task */
    tcb_t *first = sched_pick_next();
    if (first) {
        first->state = TASK_RUNNING;
        extern tcb_t *task_current_ptr;
        task_current_ptr = first;

        /* We need to switch to the first task */
        extern uint32_t *_current_task_sp;
        _current_task_sp = first->sp;

        /* Jump to first task by restoring its context */
        /* This is done through _context_switch with no previous task */
        extern void _context_switch(uint32_t **old_sp, uint32_t *new_sp);
        _context_switch((uint32_t **)0, first->sp);
    }

    /* Should never reach here */
    uart_puts("[kernel] ERROR: scheduler returned!\r\n");
    while (1) {
        xtensa_nop();
    }
}

/* Weak default app_main */
__attribute__((weak))
void app_main(void)
{
    miniprintf("[app] Default app_main() - override in apps/main.c\r\n");
}
