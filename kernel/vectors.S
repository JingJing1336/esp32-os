/*
 * ESP32 Mini Kernel - Exception/Interrupt Vector Table
 *
 * Xtensa LX6 vector table layout (offset from VECBASE):
 *   0x000: WindowOverflow4
 *   0x040: WindowUnderflow4
 *   0x080: WindowOverflow8
 *   0x0C0: WindowUnderflow8
 *   0x100: WindowOverflow12
 *   0x140: WindowUnderflow12
 *   0x180: Level 1 interrupt
 *   0x1C0: Level 2-5 / Double exception
 *   0x1E0: Level 6 / Debug
 *   0x1F0: Level 7 / NMI
 *
 * Each slot is 4 instructions (16 bytes).
 * We jump to full handlers since 4 instructions is not enough.
 */

    .section .text, "ax", @progbits

/* ============================================================
 * Vector table - must be aligned and contiguous
 * ============================================================ */
    .balign  0x200
    .global  _vector_table
_vector_table:

/* --- 0x000: WindowOverflow4 --- */
    .balign 0x40
    j   _WindowOverflow4_handler

/* --- 0x040: WindowUnderflow4 --- */
    .balign 0x40
    j   _WindowUnderflow4_handler

/* --- 0x080: WindowOverflow8 --- */
    .balign 0x40
    j   _WindowOverflow8_handler

/* --- 0x0C0: WindowUnderflow8 --- */
    .balign 0x40
    j   _WindowUnderflow8_handler

/* --- 0x100: WindowOverflow12 --- */
    .balign 0x40
    j   _WindowOverflow12_handler

/* --- 0x140: WindowUnderflow12 --- */
    .balign 0x40
    j   _WindowUnderflow12_handler

/* --- 0x180: Level 1 Interrupt --- */
    .balign 0x40
    /* Save context and jump to C handler */
    j   _level1_interrupt_handler

/* --- 0x1C0: Level 2-5 / Double Exception --- */
    .balign 0x40
    j   _default_exception_handler

/* --- 0x1E0: Level 6 / Debug --- */
    .balign 0x20
    j   _default_exception_handler

/* --- 0x1F0: Level 7 / NMI --- */
    .balign 0x10
    j   _default_exception_handler


/* ============================================================
 * Level 1 Interrupt Handler
 * ============================================================ */
    .balign 4
    .global _level1_interrupt_handler
    .type   _level1_interrupt_handler, @function
_level1_interrupt_handler:

    /* Save minimal context on stack using call0 ABI */
    /* We save a0-a3, a12-a15 (callee-saved in windowed, but let's be safe) */
    addi    sp, sp, -48

    s32i    a0, sp, 0
    s32i    a1, sp, 4
    s32i    a2, sp, 8
    s32i    a3, sp, 12
    s32i    a12, sp, 16
    s32i    a13, sp, 20
    s32i    a14, sp, 24
    s32i    a15, sp, 28

    /* Save SAR (shift amount register) */
    rsr     a2, SAR
    s32i    a2, sp, 32

    /* Save EPC1 (return address from interrupt) */
    rsr     a2, EPC1
    s32i    a2, sp, 36

    /* Save PS */
    rsr     a2, PS
    s32i    a2, sp, 40

    /* Save current SP in global for context switching */
    .global _current_task_sp
    movi    a2, _current_task_sp
    s32i    sp, a2, 0

    /* Call C interrupt dispatch */
    call0   irq_dispatch

    /* Restore SP from global (may have changed if context switched) */
    movi    a2, _current_task_sp
    l32i    sp, a2, 0

    /* Restore context */
    l32i    a2, sp, 40
    wsr     a2, PS

    l32i    a2, sp, 36
    wsr     a2, EPC1

    l32i    a2, sp, 32
    wsr     a2, SAR

    l32i    a15, sp, 28
    l32i    a14, sp, 24
    l32i    a13, sp, 20
    l32i    a12, sp, 16
    l32i    a3, sp, 12
    l32i    a2, sp, 8
    l32i    a1, sp, 4
    l32i    a0, sp, 0

    addi    sp, sp, 48

    /* Return from interrupt */
    rfi     1

    .size _level1_interrupt_handler, . - _level1_interrupt_handler


/* ============================================================
 * Default Exception Handler (hang on unexpected exception)
 * ============================================================ */
    .balign 4
    .global _default_exception_handler
_default_exception_handler:
    /* Read EXCCAUSE for debugging */
    rsr     a2, EXCCAUSE
    /* Loop forever */
    j       _default_exception_handler


/* ============================================================
 * Window overflow/underflow handlers (minimal stubs)
 * Since we use call0 ABI, these should never be called.
 * ============================================================ */
    .balign 4
    .global _WindowOverflow4_handler
_WindowOverflow4_handler:
    j       _start_hang

    .global _WindowUnderflow4_handler
_WindowUnderflow4_handler:
    j       _start_hang

    .global _WindowOverflow8_handler
_WindowOverflow8_handler:
    j       _start_hang

    .global _WindowUnderflow8_handler
_WindowUnderflow8_handler:
    j       _start_hang

    .global _WindowOverflow12_handler
_WindowOverflow12_handler:
    j       _start_hang

    .global _WindowUnderflow12_handler
_WindowUnderflow12_handler:
    j       _start_hang


/* ============================================================
 * Context switch entry point (called by scheduler)
 * Switches from current task to next task
 * ============================================================ */
    .balign 4
    .global _context_switch
    .type   _context_switch, @function
_context_switch:
    /*
     * a2 = pointer to old task's saved SP
     * a3 = new task's saved SP value
     *
     * Save callee-saved registers for call0 ABI: a0, a1, a12-a15
     */
    addi    sp, sp, -24
    s32i    a0, sp, 0
    s32i    a12, sp, 4
    s32i    a13, sp, 8
    s32i    a14, sp, 12
    s32i    a15, sp, 16

    /* Save SAR */
    rsr     a0, SAR
    s32i    a0, sp, 20

    /* Save current SP to old task's TCB */
    s32i    sp, a2, 0

    /* Load new task's SP */
    mov     sp, a3

    /* Restore SAR */
    l32i    a0, sp, 20
    wsr     a0, SAR

    /* Restore registers */
    l32i    a15, sp, 16
    l32i    a14, sp, 12
    l32i    a13, sp, 8
    l32i    a12, sp, 4
    l32i    a0, sp, 0

    addi    sp, sp, 24

    ret

    .size _context_switch, . - _context_switch


/* ============================================================
 * Task first-time entry wrapper
 * When a new task starts, its stack is set up so it "returns"
 * to this function, which calls the task function.
 * ============================================================ */
    .balign 4
    .global _task_entry
    .type   _task_entry, @function
_task_entry:
    /*
     * a2 contains the task function pointer (set during task creation).
     * Call the task function.
     */
    callx0  a2

    /* If the task function returns, delete the task */
    call0   task_exit

    /* Should never reach here */
    j       _start_hang

    .size _task_entry, . - _task_entry
