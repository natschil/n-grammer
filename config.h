//(c) 2013 Nathanael Schilling
//File for easy configuration of various options
#ifndef NGRAMCOUNTER_CONFIG_H
#define NGRAMCOUNTER_CONFIG_H

/* Options for core n-gram counter*/

	//The number of bytes to use per word. Any words larger than this are ignred.
	#ifndef MAX_WORD_SIZE
		#define MAX_WORD_SIZE 40
	#endif

	//This isn't so much of an option, but something that various parts of the program rely on. Hence changing it does nothing
	//and the setting is included only for documenation. Search for MARKER5 to find instances where code depends on this:
	#ifndef MAX_NGRAM_SIZE
		#define MAX_NGRAM_SIZE 999
	#endif

/* Options for memory management*/

	/* The N-gram counter uses (at most) MAX_CONCURRENT_BUFFERS buffers of size BUFFER_SIZE*/
	/* Note that the actual memory used is higher than what is in these buffers */
	/* Some options related to memory management are also found in the merger options below*/

	//keep this value below 4Gb, or strange things might happen.
	#ifndef BUFFER_SIZE
		#define BUFFER_SIZE (50*1024*1024)
	#endif
	//Maximum number of Buffers that should concurrently exist at any one time. Set this to the number of threads on your system + 1;
	#ifndef MAX_CONCURRENT_BUFFERS
		#define MAX_CONCURRENT_BUFFERS 7
	#endif

/*Options for Indices*/

	/*Using the --wordsearch-index-upto=n flag, it is possible to split a file into n groups to allow searching for any combination
	 * of the groups. However, we generate n choose floor(n/2) indices, which grows quickly. Generally we don't want that, so we set 
	 * the maximum n can be to MAX_INDICES. Change with care, noting that the default value of 9 corresponds to 126 indices, 10 would
	 * generate 252 indices. Check whether you have enough disk space before changing anything there, as each index can be upto the size
	 * of the input data times the n-gram length
	 */
	#ifndef MAX_INDICES
		#define MAX_INDICES 9
	#endif

/*Options for the merger*/
	//Note that when changing the values below, change them so that  MAX_K = log2(MAX_BUFFERS) + 1
	//The MAX_BUFFERS option gives the maximum number of total buffers that will be used
	#ifndef MAX_BUFFERS
		#define MAX_BUFFERS  2048
	#endif
	#ifndef MAX_K
		#define MAX_K 12
	#endif


#endif
