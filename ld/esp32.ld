/*
 * ESP32 Linker Script - Mini Kernel
 * Memory layout: Code in IRAM, Data/BSS/Heap in DRAM
 */

/* ESP32 Memory Regions */
MEMORY
{
    /* Internal SRAM for code (IRAM) - ~128KB usable */
    iram0_0_seg (RX) : org = 0x40080000, len = 0x00020000

    /* Internal SRAM for data (DRAM) - ~176KB usable */
    dram0_0_seg (RW) : org = 0x3FFB0000, len = 0x0002C000
}

/* Entry point */
ENTRY(_start)

SECTIONS
{
    /* ---- Code in IRAM ---- */
    .iram.text :
    {
        /* Start code must be first */
        *(.text.entry)
        *(.text .text.*)
        *(.literal .literal.*)
        *(.rodata .rodata.*)
    } > iram0_0_seg

    /* ---- Data in DRAM ---- */
    .dram.data :
    {
        __data_start = .;
        *(.data .data.*)
        __data_end = .;
    } > dram0_0_seg

    /* ---- BSS (zero-initialized) ---- */
    .dram.bss :
    {
        . = ALIGN(4);
        __bss_start = .;
        *(.bss .bss.*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end = .;
    } > dram0_0_seg

    /* ---- Main stack (grows downward) ---- */
    .stack (NOLOAD) :
    {
        . = ALIGN(16);
        __stack_top = .;
        . = . + 0x2000;  /* 8KB main stack */
        __stack_bottom = .;
    } > dram0_0_seg

    /* ---- Heap (remaining DRAM) ---- */
    .heap (NOLOAD) :
    {
        . = ALIGN(4);
        __heap_start = .;
        . = . + 0x10000;  /* 64KB heap */
        __heap_end = .;
    } > dram0_0_seg

    /* Discard unwanted sections */
    /DISCARD/ :
    {
        *(.comment)
        *(.note.*)
        *(.eh_frame*)
        *(.ARM.*)
    }
}

/* Provide symbols for startup code */
PROVIDE(_bss_start = __bss_start);
PROVIDE(_bss_end = __bss_end);
PROVIDE(_stack_top = __stack_top);
PROVIDE(_heap_start = __heap_start);
PROVIDE(_heap_end = __heap_end);
