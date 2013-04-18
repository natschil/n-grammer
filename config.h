//(c) 2013 Nathanael Schilling
//File for easy configuration of various options
#ifndef NGRAMCOUNTER_CONFIG_H
#define NGRAMCOUNTER_CONFIG_H

/* Options for core n-gram counter*/

	//The number of bytes to use per word. Any words larger than this are ignred.
	#ifndef MAX_WORD_SIZE
		#define MAX_WORD_SIZE 40
	#endif

/* Options for memory management*/

	/* The N-gram counter uses two buffers that consist of NUM_PAGES "pages" where each "page" is of size MEMORY_PAGE_SIZE*/
	/* Note that the actual memory used is slightly higher than 2 * MEMORY_PAGE_SIZE * NUM_PAGES */
	/* Some options related to memory management are also found in the merger options below*/

	//The memory size per "page"
	#ifndef MEMORY_PAGE_SIZE
		#define MEMORY_PAGE_SIZE (4*1024*1024)
	#endif
	//Set this to half of the actual number of "pages" used. 
	#ifndef NUM_PAGES
		#define NUM_PAGES 10
	#endif

/*Options for the merger*/
	//Note that when changing the values below, change them so that  MAX_K = log2(MAX_BUFFERS) + 1
	#ifndef MAX_BUFFERS
		#define MAX_BUFFERS  2048
	#endif
	#ifndef MAX_K
		#define MAX_K 12
	#endif

#endif
