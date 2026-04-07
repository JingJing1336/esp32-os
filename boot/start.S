/*
 * ESP32 Mini Kernel - Startup Code
 *
 * Entry point for the kernel. Sets up stack pointer,
 * clears BSS section, installs vector table, and
 * jumps to kernel_main().
 *
 * Uses call0 ABI (no windowed register handling needed).
 */

    .section .text.entry, "ax", @progbits
    .global _start
    .type   _start, @function
    .align  4

_start:
    /*
     * Disable interrupts and windowed register options.
     * Set PS: INTLEVEL=15 (all disabled), clear other bits.
     */
    movi    a3, 0x3F
    wsr     a3, PS
    rsync

    /* Disable all interrupt sources */
    movi    a3, 0
    wsr     a3, INTENABLE
    rsync

    /* Set up stack pointer to top of main stack */
    movi    sp, _stack_top

    /*
     * Install our own exception vector table.
     * Set VECBASE to point to _vector_table defined in vectors.S
     */
    movi    a3, _vector_table
    wsr     a3, VECBASE
    rsync

    /*
     * Clear BSS section
     * a6 = start address, a7 = end address
     */
    movi    a6, _bss_start
    movi    a7, _bss_end

.Lbss_loop:
    blt     a6, a7, .Lbss_clear
    j       .Lbss_done

.Lbss_clear:
    s32i    a0, a6, 0       /* store zero (a0 was set to 0 above implicitly) */
    addi    a6, a6, 4
    j       .Lbss_loop

.Lbss_done:
    /* Make sure a0 = 0 for call0 ABI */
    movi    a0, 0

    /* Call kernel_main() */
    call0   kernel_main

    /* If kernel_main returns, hang */
    .global _start_hang
_start_hang:
    j       _start_hang

    .size _start, . - _start
