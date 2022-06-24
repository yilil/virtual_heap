#include "virtual_alloc.h"
#include "virtual_sbrk.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void init_allocator(void * heapstart, uint8_t initial_size, uint8_t min_size) {
    // Your code here
    int32_t increSize = (int32_t) pow_2(initial_size);
    virtual_sbrk(sizeof(struct virtual_heap) + increSize + sizeof(struct Block));
    
    struct virtual_heap* heap = (struct virtual_heap*) heapstart;

    heap->size = pow_2(initial_size);
    heap->min_size = pow_2(min_size);

    heap->startBlock = heapstart + sizeof(struct virtual_heap) + heap->size;
    
    struct Block* block = heap->startBlock;
    block->size = pow_2(initial_size);
    block->status = 0;

    block->left = NULL;
    block->right = NULL;
    
    block->block_address = heapstart + sizeof(struct virtual_heap);

    heap->block_Count = 1;
}

struct Block * split(void * heapstart, struct Block * block){
    struct virtual_heap* heap = (struct virtual_heap*) heapstart;
    
    //Intialise a new block on the actual heap, i.e. the right block after split
    struct Block* new_block = heap->startBlock + heap->block_Count;
    new_block->size = block->size/2;
    new_block->status = 0;
    
    new_block->left = block;
    new_block->right = block->right;
    if(block->right != NULL){
        block->right->left = new_block;
    }
    
    new_block->block_address = block->block_address + new_block->size;
    heap->block_Count++;

    block->size /= 2;
    block->status = 1;
    block->right = new_block;
    
    return block;
}

void merge(void * heapstart){
    struct virtual_heap* heap = (struct virtual_heap*) heapstart;
    struct Block* current_block = heap->startBlock;

    while(current_block->right != NULL){
        struct Block* next_block = current_block->right;
        if(current_block->status == 1){
            current_block = next_block;
            continue;
        }
        
        if(next_block != NULL && next_block->status == 0 && next_block->size == current_block->size){
            current_block->size *= 2;
            current_block->status = 0;
            
            next_block = next_block->right;
            current_block->right = next_block;
            if(next_block != NULL){
                next_block->left = current_block;
            }
            //If merged, we will go back to start and check for new merges, also decrease size needed for a block
            current_block = heap->startBlock;
            heap->block_Count--;
            heap->spare_Count++;
            continue;
        }
        current_block = current_block->right;
    }
}

void * virtual_malloc(void * heapstart, uint32_t size) {
    struct virtual_heap* heap = (struct virtual_heap*) heapstart;
    
    if(size == 0){
        return NULL;
    }

    uint64_t required = size;
    //Find the minimum 2^i size needed for the required size
    if(size < heap->min_size){
        required = heap->min_size;
    }else{
        int i = 0;
        while(pow_2(i) < required){
            i++;
        }
        required = pow_2(i);
    }

    //block_ptr points to the address of each of the block on the actual heap, starting from the first block, then left to right
    struct Block* block_ptr = heap->startBlock;
    //block points to the address of the first suitable size block
    struct Block* block = heap->startBlock;

    int block_found = 0;
    while(block_ptr!= NULL){
        if(block_ptr->size == required && block_ptr->status == 0){
            block_ptr->status = 1;
            return block_ptr->block_address;
        }else if(block_found == 0 && block_ptr->size > required && block_ptr->status == 0){
            block = block_ptr;
            block_found = 1;
        }
        block_ptr = block_ptr->right;
    }
    
    if(!block_found){
        return NULL;
    }
    while(block->size > required){
        //When we split a block into two, we would need to call virtual_sbrk to increase our memory to store information about the additional block
        virtual_sbrk(sizeof(struct Block));
        block = split(heapstart, block);
    }
    return block->block_address;
}

int virtual_free(void * heapstart, void * ptr) {
    struct virtual_heap* heap = (struct virtual_heap*) heapstart;
    
    //Block pointer on the actual heap
    struct Block* current_block = heap->startBlock;

    int block_found = 0;
    while(current_block != NULL){
        if(current_block->block_address == ptr){
            block_found = 1;
            break;
        }
        current_block = current_block->right;
    }
    
    //If ptr does not point to a previous allocated block, return non-zero
    if(!block_found){
        return 1;
    }

    current_block->status = 0;
    merge(heapstart);
    return 0;
}

void * virtual_realloc(void * heapstart, void * ptr, uint32_t size) {
    struct virtual_heap* heap = (struct virtual_heap*) heapstart;
    //Make a copy of our virtual heap
    struct virtual_heap heap_reset = *(heap);

    if(ptr == NULL){
        return virtual_malloc(heapstart, size);
    }
    
    if(size == 0 && ptr == NULL){
        virtual_free(heapstart, ptr);
        return NULL;
    }
    //The size of all blocks in our linked list, including the ones we are using and the ones that were 'freed'
    int64_t block_area = sizeof(struct Block)*(heap->block_Count+heap->spare_Count);
    //Allocate memory to store a copy of our blocks
    virtual_sbrk(block_area);
    memcpy(heapstart + sizeof(struct virtual_heap) + heap->size + block_area, heapstart + sizeof(struct virtual_heap) + heap->size , block_area);
    
    //If we try to realloc a block that was not previously allocated, return NULL
    if(virtual_free(heapstart, ptr) != 0){
        return NULL;
    }

    void* block = virtual_malloc(heapstart, size);
    //If malloc failed, e.g. no suitable free block for allocation, reset our virtual heap and block area
    if(block == NULL){
        *heap = heap_reset;
        memcpy(heapstart + heap->size + sizeof(struct virtual_heap), heapstart + sizeof(struct virtual_heap) + heap->size + block_area, block_area);
        virtual_sbrk(-block_area);
        return NULL;
    }else{
        uint64_t ptr_size = 0;
        struct Block* current_block = heap->startBlock;
        while(current_block != NULL){
            //Look for the size of the previous allocated block pointed by ptr
            if(current_block->block_address == ptr){
                ptr_size = current_block->size;
                break;
            }
            current_block = current_block->right;
        }

        uint64_t truncated = ptr_size>size ? size : ptr_size;
        memmove(block, ptr, truncated);
        virtual_sbrk(-block_area);
        return block;
    }
    return NULL;
}

void virtual_info(void * heapstart) {
    struct virtual_heap* heap = (struct virtual_heap*) heapstart;
    struct Block* current_block = heap->startBlock;
    while(current_block != NULL){
        printf("%s %ld\n", current_block->status ? "allocated" : "free", current_block->size);
        current_block = current_block->right;
    }
}