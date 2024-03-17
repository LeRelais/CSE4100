/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your information in the following struct.
 ********************************************************/
team_t team = {
    /* Your student ID */
    "20181625",
    /* Your full name*/
    "Hyeonjun Nam",
    /* Your email address */
    "lerelais0102@naver.com",
};

void *list = NULL; //free list

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define WORD 4
#define DWORD 8
#define CHUNKSIZE (1<<12)

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

void removefromlist(void *node) {
    if (node == list) { 
        list = *(void **)(node + WORD); 
    }
    else {
        void *prev = *(void **)(node);
        void *next = *(void **)(node + WORD);

        *(void **)(prev + WORD) = next; 
        *(void **)(next) = prev;
    }
}

size_t allocateflag(void *node) {
    return *(size_t *)(node - WORD) & 1;
}

size_t returnblocksize(void *node) {
    size_t lastbit = allocateflag(node);
    if(lastbit == 1)
     return *(unsigned int *)(node - WORD) - 1;
    else
     return *(unsigned int *)(node - WORD);
}

void split(void *node, size_t size) {
    size_t cursize = returnblocksize(node);
    size_t blocksize_aftersplit = cursize - size;
    removefromlist(node); 

    if (blocksize_aftersplit >= DWORD * 2) { 
         *(size_t *)(node - WORD) = size | 1;
         *(size_t *)(node + size - DWORD) = size | 1;

        void *next = node + size;
        *(size_t *)(next - WORD) = blocksize_aftersplit | 0;
        *(size_t *)(next + blocksize_aftersplit - DWORD) = blocksize_aftersplit | 0;
        
        *(void **)(next + WORD) = list;
        *(void **)(list) = next;

        list = next;
    }
    else{
        *(size_t *)(node - WORD) = cursize | 1;
        *(size_t *)(node + cursize - DWORD) = cursize | 1;
    }
}

void *mm_fit(size_t size) {
    void *cur;
    for(cur = list; cur != NULL; cur = *(void **)(cur + WORD)){
        if((size <= returnblocksize(cur)) && !allocateflag(cur)){
            return cur;
        }
    }

    return NULL;
}

void *merge(void *node) {
    void *prev = node - returnblocksize(node - WORD);
    void *next = node + returnblocksize(node);

    if(allocateflag(prev) && allocateflag(next)){
        *(void **)(node + WORD) = list;      
        *(void **)(list) = node;                  
        list = node;  

        return node;                          
    }
    else if (allocateflag(prev) && !allocateflag(next)) { 
        size_t newsize = returnblocksize(node) + returnblocksize(next);
        removefromlist(next);

        *(size_t *)(node - WORD) = newsize | 0; 
        *(size_t *)(next + returnblocksize(next) - DWORD) = newsize | 0;

        *(void **)(node + WORD) = list;      
        *(void **)(list) = node;                  
        list = node;  

        return node;   
    }
    else if (!allocateflag(prev) && allocateflag(next)) { 
        size_t newsize = returnblocksize(prev) + returnblocksize(node);
        removefromlist(prev);

        *(size_t *)(prev - WORD) = newsize | 0;
        *(size_t *)(node + returnblocksize(node) - DWORD) = newsize | 0;

        *(void **)(prev + WORD) = list;
        *(void **)(list) = prev;
        list = prev;

        return prev;
    }
    else{
        size_t newsize = returnblocksize(prev) + returnblocksize(node) + returnblocksize(next);
        removefromlist(prev);
        removefromlist(next);

        *(size_t *)(prev - WORD) = newsize | 0;
        *(size_t *)(next + returnblocksize(next) - DWORD) = newsize | 0;

        *(void **)(prev + WORD) = list;
        *(void **)(list) = prev;
        list = prev;

        return prev;
    }
}

void *extend_heap(size_t size) {
    
    void *ptr = mem_sbrk(size);
    if (ptr == (void *)-1) return NULL;

    *(size_t *)(ptr - WORD) = size | 0; 
    *(size_t *)(ptr + size - DWORD) = size | 0;

    *(size_t *)(ptr + returnblocksize(ptr) - WORD) = 0 | 1; 

    return merge(ptr); 
}


/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void) {
    void* ptr = mem_sbrk(6*WORD); 
    if (ptr == (void *)-1) return -1; 

    *(size_t *)(ptr) = 0;
    *(size_t *)(ptr + WORD) = DWORD*2 | 1;
    *(size_t *)(ptr + (2*WORD)) = 0;
    *(size_t *)(ptr + (3*WORD)) = 0;
    *(size_t *)(ptr + (4*WORD)) =  DWORD*2 | 1;
    *(size_t *)(ptr + (5*WORD)) = 0 | 1;
    
    list = ptr + 2*WORD; 

    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    if (size <= 0) return NULL;

    size_t alignsize = ALIGN(size + SIZE_T_SIZE);
    void *tmp = mm_fit(alignsize);

    if (tmp != NULL) { 
        split(tmp, alignsize);
        return tmp;
    }
    else{
        size_t heapsize = alignsize;

        if(heapsize < CHUNKSIZE)
            heapsize = CHUNKSIZE;

        tmp = extend_heap(heapsize);
        if (tmp == NULL) 
            return NULL;

        split(tmp, alignsize);
        return tmp; 
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */

void mm_free(void *node) {
    *(size_t *)(node - WORD) = returnblocksize(node) | 0; 
    *(size_t *)(node + returnblocksize(node) - DWORD) = returnblocksize(node) | 0;

    merge(node); 
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    if (size <= 0) 
        return NULL;

    if (ptr == NULL) {
        ptr = mm_malloc(size);
        return ptr;
    }
    else{
        size_t block_size = returnblocksize(ptr);
        if(block_size >= size)
            size = block_size;

        void *newptr = mm_malloc(size); 
        if (newptr == NULL) 
            return NULL;

        memcpy(newptr, ptr, block_size); 
        mm_free(ptr); 

        return newptr;
    }
}