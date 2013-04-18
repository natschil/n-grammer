//(c) 2013 Nathanael Schilling

#include "memory_management.h"


static void* pages[2][NUM_PAGES] = {{NULL}};
omp_lock_t locks[2];
volatile size_t current_page_group = 0;
//volatile int page_has_been_swapped[2] = {1};
static size_t current_page = 0;
static size_t current_page_occupied = 0;
static size_t spare_space_at_end = 0;

void (*buffer_switching_callback)(void) = NULL;


void init_permanent_malloc(void (*callback)(void),size_t maximum_single_allocation)
{
	//We allocate all pages at once: if we're going to run out of memory we want to know so before we do any processing.
	int i;
	for(i = 0; i<2;i++)
	{
		int j;
		for(j= 0; j<NUM_PAGES;j++)
		{
			if(!(pages[i][j] = malloc(MEMORY_PAGE_SIZE)))
			{
				fprintf(stderr,"Error allocating memory at page %d \n",(int) i * j);
			}
		}

		#ifdef _OPENMP
		omp_init_lock(&(locks[i]));
		#endif
	}
	current_page_group = 0;
	current_page = 0;
	buffer_switching_callback = callback;
	spare_space_at_end = maximum_single_allocation;
	#pragma omp flush
	return;
}

/*This function allocates space from the current buffer.
 * In the NGram counter, it should *only* be called from fillABuffer or functions it calls.
 */
void* permanently_malloc(size_t numbytes,int* retval)
{
	#pragma omp flush(current_page_group,current_page,current_page_occupied)
	if(( current_page_occupied += numbytes )<=(size_t) MEMORY_PAGE_SIZE) //i.e. if there is enough memory available.
	{
		#pragma omp flush(current_page_occupied)
		if(current_page == (NUM_PAGES - 1))//On the last page
		{
			if(current_page_occupied > (MEMORY_PAGE_SIZE - spare_space_at_end))
				*retval = -1;
		}
		return pages[current_page_group][current_page] + (current_page_occupied - numbytes);
	}else
	{
		current_page++;
		current_page_occupied = 0;	
		#pragma omp flush(current_page)
		if(current_page < NUM_PAGES)
		{
			current_page_occupied = numbytes;
			#pragma omp flush(current_page_occupied)
			return pages[current_page_group][current_page];
		}else //This should never happen, as buffer switching should be done manually
		{
			fprintf(stderr,"Not enough memory in this buffer, adjust what you pass to init_permanent_malloc() as the first parameter\n");
			exit(-1);

			//We wait for the next page to have been cleared:
			//MARKER4
						return pages[current_page_group][current_page];
		}
	}
	return NULL;
}
void switch_permanent_malloc_buffers(void)
{
	#pragma omp flush(current_page_group,current_page,current_page_occupied)
	setpagelock(!current_page_group);

	current_page = 0;	
	current_page_occupied = 0;
	current_page_group = !current_page_group; 
	#pragma omp flush(current_page_group,current_page,current_page_occupied)

	if(buffer_switching_callback)
		(*buffer_switching_callback)();

	unsetpagelock(current_page_group);
	return;
}

void rewind_permanent_malloc(size_t numbytes)
{
	if(current_page_occupied >= numbytes)
		current_page_occupied -= numbytes;
}

void free_all_pages()
{
	int i;
	for(i = 0; i< 2; i++)
	{
		int j;
		for(j = 0; j < NUM_PAGES; j++)
		{
			free(pages[i][j]);
		}
	}
	return;
}

void setpagelock(size_t pagenum)
{
	#ifdef _OPENMP
	omp_set_lock(&locks[pagenum]);
	#endif
}

void unsetpagelock(size_t pagenum)
{
	#ifdef _OPENMP
	omp_unset_lock(&locks[pagenum]);
	#endif
}
