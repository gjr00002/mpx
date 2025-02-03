#ifndef MPX_MEMORY_H
#define MPX_MEMORY_H

#include <stddef.h>

/**
 @file memory.h
 @brief MPX-specific dynamic memory functions.
*/

/**
 Allocate dynamic memory.
 @param size The amount of memory, in bytes, to allocate
 @return NULL on error, otherwise the address of the newly allocated memory
*/
void *sys_alloc_mem(size_t size);

/**
 Free dynamic memory.
 @param ptr The address of dynamically allocated memory to free
 @return 0 on success, non-zero on error
*/
int sys_free_mem(void *ptr);

/**
 Installs user-supplied heap management functions.
 @param alloc_fn A function that dynamically allocates memory
 @param free_fn A function that frees dynamically allocated memory
*/
void sys_set_heap_functions(void * (*alloc_fn)(size_t), int (*free_fn)(void *));

void initialize_heap(size_t sz);
void *allocate_memory(size_t sz);
int free_memory(void *ptr);

struct mcb {
	size_t size;    // LSB ? FREE : ALLOC; LSB2 ? END : NOTEND
	char start[];
};

extern struct mcb *memlist;
struct mcb *mcb_next(struct mcb *mcb);

#define FREE    (1<<0)
#define END     (1<<1)

#define mcb_size(m)		((m)->size & (~(END|FREE)))
#define mcb_isfree(m)		((m)->size & FREE)
#define mcb_isend(m)		((m)->size & END)
#define mcb_setfree(m)		((m)->size |= FREE)
#define mcb_clrfree(m)		((m)->size &= ~FREE)
#define mcb_copyend(m, n)	((m)->size |= mcb_isend(n) ? END : 0)

#endif
