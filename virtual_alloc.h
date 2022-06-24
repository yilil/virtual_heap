#ifndef VIRTUAL_ALLOC_H
#define VIRTUAL_ALLOC_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

void init_allocator(void * heapstart, uint8_t initial_size, uint8_t min_size);

void * virtual_malloc(void * heapstart, uint32_t size);

int virtual_free(void * heapstart, void * ptr);

void * virtual_realloc(void * heapstart, void * ptr, uint32_t size);

void virtual_info(void * heapstart);

uint64_t pow_2(uint64_t x)
{
  return (1 << x);
}

struct Block{
    //Size of block
    uint64_t size;
    //Whether the block is allocated or not. 0 = free to allocate, 1 = already allocated
    char status;
    //Left block buddy
    struct Block* left;
    //Right block buddy
    struct Block* right;
    //Address of the block on the virtual heap
    void* block_address;
};

struct virtual_heap{
    //Size of the virtual heap 
    int64_t size;
    //Min size for splitting
    int64_t min_size;
    //Pointer to the first block on the actual heap
    struct Block* startBlock;
    //Total number of blocks
    int64_t block_Count;
    //Total number of spare blocks after merge
    int64_t spare_Count;
};

#endif
