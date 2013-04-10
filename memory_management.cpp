//(c) 2013 Nathanael Schilling

#include "memory_management.h"


static void* pages[2][NUM_PAGES] = {{NULL}};
static size_t current_page_group = 0;
static size_t current_page = 0;
static size_t current_page_occupied = 0;
int safe_to_switch;

void (*to_run_when_swapping_buffers)() = NULL;

void init_permanent_malloc(void (*to_run)())
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
	}
	current_page_group = 0;
	current_page = 0;
	safe_to_switch = 1;
	to_run_when_swapping_buffers = to_run;
	return;
}
void* permanently_malloc(size_t numbytes)
{
	if(( current_page_occupied += numbytes )<(size_t) MEMORY_PAGE_SIZE) //i.e. if there is enough memory available.
	{
		return pages[current_page_group][current_page] + (current_page_occupied - numbytes);
	}else
	{
		current_page++;
		current_page_occupied = 0;	
		if(current_page < NUM_PAGES)
		{
			return permanently_malloc(numbytes);
		}else
		{
			current_page = 0;	
			current_page_occupied = 0;
			current_page_group = !current_page_group;

			if(to_run_when_swapping_buffers)
				to_run_when_swapping_buffers();
			while(!safe_to_switch)
				usleep(10);

			return permanently_malloc(numbytes);
		}
	}
	return 0;
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
