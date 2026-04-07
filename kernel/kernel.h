#ifndef KERNEL_H
#define KERNEL_H

#include "types.h"

/* Main kernel entry point (called from start.S) */
void kernel_main(void);

/* Kernel heap management */
void  kheap_init(void *start, uint32_t size);
void *kmalloc(uint32_t size);
void  kfree(void *ptr);

#endif /* KERNEL_H */
