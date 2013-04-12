// (c) 2013 Nathanael Schilling
// This file contains functions for (somewhat) efficiently allocating large amounts of memory
// but without the ability to free small amounts of memory.

//The global constants below should be modified before actually running.

#ifndef MEMORY_MANAGMENT_H
#define MEMORY_MANAGEMENT_H

#include "stdlib.h"
#include <stdio.h>
#include <unistd.h>


//2M per memory "page" size.
//Edit: changed 1M to make it really small
#define MEMORY_PAGE_SIZE (4 * 1024 * 1024)
//Set this to half of the actual number of "pages" used. Because of the way buffer swapping is used and concurrency, etc...
#define NUM_PAGES 3
//Therefore
#define TOTAL_MEM_USED MEMORY_PAGE_SIZE * 2 * NUM_PAGES

void init_permanent_malloc(void(*)(void));
extern volatile int safe_to_switch;

void* permanently_malloc(size_t size,int* swapbuffer_retval);

void free_all_pages(void);


#endif
