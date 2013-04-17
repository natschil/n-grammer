// (c) 2013 Nathanael Schilling
//
// This file contains functions for (somewhat) efficiently allocating large amounts of memory
// but without the ability to free small amounts of memory.

//The global constant MEMORY_PAGE_SIZE and NUM_PAGES below should be modified before actually running.
//The N-gram counter uses buffers to store word-ngrams. However, because the stl map containers used
//to keep track n-grams do not use the buffers for memory, actual memory usage is higher.

#ifndef MEMORY_MANAGMENT_H
#define MEMORY_MANAGEMENT_H

#include "stdlib.h"
#include <stdio.h>
#include <unistd.h>
#include <omp.h>


//Set this to half of the actual number of "pages" used. 
//4M per memory "page" size.
#define MEMORY_PAGE_SIZE (4*1024*1024)
//Set this to the number of "pages used"
#define NUM_PAGES 100
//Because we use two buffers that take up MEMORY_PAGE_SIZE*NUM_PAGES memory
//Therefore
#define TOTAL_MEM_USED MEMORY_PAGE_SIZE * 2 * NUM_PAGES
//Note that the variable above is used only for documentation and should not be changed.

void init_permanent_malloc(void(*)(void));
extern volatile size_t current_page_group;
//extern volatile int page_has_been_swapped[2];

void* permanently_malloc(size_t size,int* swapbuffer_retval);

void rewind_permanent_malloc(size_t numbytes);

void free_all_pages(void);

void setpagelock(size_t);
void unsetpagelock(size_t);


#endif
