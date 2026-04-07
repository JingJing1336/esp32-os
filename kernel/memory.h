#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"

/* Block header for heap allocator */
typedef struct mem_block {
    uint32_t            size;   /* Size of the block (excluding header) */
    uint32_t            used;   /* 0 = free, 1 = allocated */
    struct mem_block   *next;   /* Next block in the heap */
} mem_block_t;

#define MEM_BLOCK_HEADER_SIZE   sizeof(mem_block_t)
#define MEM_ALIGN               4
#define MEM_ALIGN_UP(x)         (((x) + MEM_ALIGN - 1) & ~(MEM_ALIGN - 1))

void  kheap_init(void *start, uint32_t size);
void *kmalloc(uint32_t size);
void  kfree(void *ptr);
uint32_t kheap_free(void);
uint32_t kheap_used(void);

#endif /* MEMORY_H */
