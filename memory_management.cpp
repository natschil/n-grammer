//(c) 2013 Nathanael Schilling

#include "memory_management.h"


static void* pages[2][NUM_PAGES] = {{NULL}};
omp_lock_t locks[2];
volatile size_t current_page_group = 0;
//volatile int page_has_been_swapped[2] = {1};
static size_t current_page = 0;
static size_t current_page_occupied = 0;

void (*buffer_switching_callback)(void) = NULL;


void init_permanent_malloc(void (*callback)(void))
{
	//We allocate all pages at once: if we're going to run out of memory we want to know so before we do any processing.
	for(int i = 0; i<2;i++)
	{
		for(int j= 0; j<NUM_PAGES;j++)
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
	return;
}
void* permanently_malloc(size_t numbytes,int* retval)
{
	setpagelock(current_page_group);
	if(( current_page_occupied += numbytes )<(size_t) MEMORY_PAGE_SIZE) //i.e. if there is enough memory available.
	{
		unsetpagelock(current_page_group);
		return pages[current_page_group][current_page] + (current_page_occupied - numbytes);
	}else
	{
		current_page++;
		current_page_occupied = 0;	
		if(current_page < NUM_PAGES)
		{
			current_page_occupied = numbytes;
			unsetpagelock(current_page_group);
			return pages[current_page_group][current_page];
		}else
		{

			//We wait for the next page to have been cleared:
			//while(!page_has_been_swapped[!current_page_group])
			//	usleep(10);
			setpagelock(!current_page_group);

			current_page = 0;	
			current_page_occupied = numbytes;
			current_page_group = !current_page_group; //This line

			unsetpagelock(!current_page_group);//and this line are confusing, so think about them before you change anything.

			if(buffer_switching_callback)
				(*buffer_switching_callback)();

			//page_has_been_swapped[!current_page_group] = 0;

			*retval = -1;

			unsetpagelock(current_page_group);
			return pages[current_page_group][current_page];
		}
	}
	return NULL;
}

void free_all_pages()
{
	for(int i = 0; i< 2; i++)
	{
		for(int j = 0; j < NUM_PAGES; j++)
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
