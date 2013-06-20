//(c) 2013 Nathanael Schilling
//File for easy configuration of various options
#ifndef NGRAMCOUNTER_CONFIG_H
#define NGRAMCOUNTER_CONFIG_H

/* Options for core n-gram counter*/

	#ifndef NGRAM_COUNTER_VERSION
		#define NGRAM_COUNTER_VERSION 2
	#endif

	//The number of bytes to use per word. Any words larger than this are ignored.
	#ifndef MAX_WORD_SIZE
		#define MAX_WORD_SIZE 40
	#endif

	//The maximum number of bytes to use per part of speech classification
	#ifndef MAX_CLASSIFICATION_SIZE
		#define MAX_CLASSIFICATION_SIZE 40
	#endif

	//The maximum number of bytes to use per lemma.
	#ifndef MAX_LEMMA_SIZE
		#define MAX_LEMMA_SIZE 40
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

	//keep this value below THEORETICAL_MAX_BUFFER_SIZE, or strange things might happen.
	

	#ifndef MEMORY_TO_USE_FOR_BUFFERS
		#define MEMORY_TO_USE_FOR_BUFFERS (800ll*1024ll*1024ll)
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
	//Note that when changing the values below, change them so that MAX_BUFFERS is a power of 2
	#ifndef MAX_BUFFERS
		#define MAX_BUFFERS  2048
	#endif

#endif
