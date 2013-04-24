// (c) 2013 Nathanael Schilling
//
// This file contains functions for (somewhat) efficiently allocating large amounts of memory
// but without the ability to free small amounts of memory.


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
void init_permanent_malloc(size_t);
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
