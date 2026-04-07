#include "memory.h"
#include "kernel.h"

/* Heap state */
static mem_block_t *heap_first = (mem_block_t *)0;
static uint32_t heap_total = 0;

void kheap_init(void *start, uint32_t size)
{
    heap_first = (mem_block_t *)start;
    heap_total = size;

    /* Initialize as one large free block */
    heap_first->size = size - MEM_BLOCK_HEADER_SIZE;
    heap_first->used = 0;
    heap_first->next = (mem_block_t *)0;
}

/*
 * First-fit allocator:
 * Scans block list for first free block large enough.
 * Splits the block if there's enough remaining space.
 */
void *kmalloc(uint32_t size)
{
    if (size == 0)
        return (void *)0;

    /* Align size */
    size = MEM_ALIGN_UP(size);

    mem_block_t *block = heap_first;
    while (block) {
        if (!block->used && block->size >= size) {
            /* Found a suitable block */

            /* Split if remaining space is large enough for header + data */
            if (block->size >= size + MEM_BLOCK_HEADER_SIZE + MEM_ALIGN) {
                mem_block_t *new_block = (mem_block_t *)((uint8_t *)block +
                                            MEM_BLOCK_HEADER_SIZE + size);
                new_block->size = block->size - size - MEM_BLOCK_HEADER_SIZE;
                new_block->used = 0;
                new_block->next = block->next;

                block->size = size;
                block->next = new_block;
            }

            block->used = 1;
            return (void *)((uint8_t *)block + MEM_BLOCK_HEADER_SIZE);
        }
        block = block->next;
    }

    /* Out of memory */
    return (void *)0;
}

/*
 * Free a previously allocated block.
 * Merges with adjacent free blocks to reduce fragmentation.
 */
void kfree(void *ptr)
{
    if (!ptr)
        return;

    mem_block_t *block = (mem_block_t *)((uint8_t *)ptr - MEM_BLOCK_HEADER_SIZE);
    block->used = 0;

    /* Coalesce with next block if free */
    while (block->next && !block->next->used) {
        block->size += MEM_BLOCK_HEADER_SIZE + block->next->size;
        block->next = block->next->next;
    }

    /* Coalesce with previous block - scan from start */
    mem_block_t *prev = heap_first;
    while (prev && prev != block) {
        if (!prev->used && prev->next == block) {
            prev->size += MEM_BLOCK_HEADER_SIZE + block->size;
            prev->next = block->next;
            break;
        }
        /* Also check if prev is free and adjacent */
        if (!prev->used) {
            mem_block_t *expected_next = (mem_block_t *)((uint8_t *)prev +
                                        MEM_BLOCK_HEADER_SIZE + prev->size);
            if (expected_next == block) {
                prev->size += MEM_BLOCK_HEADER_SIZE + block->size;
                prev->next = block->next;
                block = prev;
            }
        }
        prev = prev->next;
    }
}

uint32_t kheap_free(void)
{
    uint32_t free = 0;
    mem_block_t *block = heap_first;
    while (block) {
        if (!block->used)
            free += block->size;
        block = block->next;
    }
    return free;
}

uint32_t kheap_used(void)
{
    uint32_t used = 0;
    mem_block_t *block = heap_first;
    while (block) {
        if (block->used)
            used += block->size;
        block = block->next;
    }
    return used;
}
