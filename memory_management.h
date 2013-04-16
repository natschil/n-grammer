// (c) 2013 Nathanael Schilling
// This file contains functions for (somewhat) efficiently allocating large amounts of memory
// but without the ability to free small amounts of memory.

//The global constants below should be modified before actually running.

#ifndef MEMORY_MANAGMENT_H
#define MEMORY_MANAGEMENT_H

#include "stdlib.h"
#include <stdio.h>
#include <unistd.h>
#include <omp.h>


//4M per memory "page" size.
//Set this to half of the actual number of "pages" used. Because of the way buffer swapping is used and concurrency, etc...
#define MEMORY_PAGE_SIZE (4 * 1024 * 1024)
#define NUM_PAGES 100 
//Therefore
#define TOTAL_MEM_USED MEMORY_PAGE_SIZE * 2 * NUM_PAGES

void init_permanent_malloc(void(*)(void));
extern volatile size_t current_page_group;
//extern volatile int page_has_been_swapped[2];

void* permanently_malloc(size_t size,int* swapbuffer_retval);

void rewind_permanent_malloc(size_t numbytes);

void free_all_pages(void);

void setpagelock(size_t);
void unsetpagelock(size_t);


#endif
