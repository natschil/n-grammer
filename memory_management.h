// (c) 2013 Nathanael Schilling
//
// This file contains functions for (somewhat) efficiently allocating large amounts of memory
// but without the ability to free small amounts of memory.

//The global constant MEMORY_PAGE_SIZE and NUM_PAGES below should be modified before actually running.
//The N-gram counter uses buffers to store word-ngrams. However, because the stl map containers used
//to keep track n-grams do not use the buffers for memory, actual memory usage is higher.

#ifndef MEMORY_MANAGMENT_H
#define MEMORY_MANAGEMENT_H
#ifdef __cplusplus
extern "C"{
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <omp.h>
#include "config.h"


//These functions should only be called from the function filling the buffer.
void init_permanent_malloc(void(*)(void),size_t);
extern volatile size_t current_page_group;

void* permanently_malloc(size_t size,int* swapbuffer_retval);
void switch_permanent_malloc_buffers(void);

void rewind_permanent_malloc(size_t numbytes);

void free_all_pages(void);

void setpagelock(size_t);
void unsetpagelock(size_t);

#ifdef __cplusplus
}
#endif
#endif
