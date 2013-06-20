/*(c) 2013 Nathanael Schilling
 * This file contains functions for keeping track of words, n-grams and indices
 */

using namespace std;

#ifndef NGRAM_COUNTING_INDICES_V2_HEADER
#define NGRAM_COUNTING_INDICES_V2_HEADER

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistr.h>
#include <unistdio.h>
#include <sys/stat.h>
#include <errno.h>


#include <iostream>
#include <vector>
#include <stack>
#include <algorithm>
#include <sstream>

#include "config.h"
#include "searchindexcombinations.h"
#include "mergefiles.h"
#include "util.h"
#include "words.h"
#include "manage_metadata.h"



/*The Buffer class represents a large region of memory that is filled from the bottom with strings and from the top with word objects
 * It provides methods to manage this memory as well as possible. 
 */
class Buffer
{
	public:
		//internal_buffer should point to a region of memory of size buffer_size
		//maximum_single_allocation is the maximum possible size of a single allocation, i.e. it should always be possible
		//to allocate maximum_single_allocation or less bytes of memory in the buffer.
		//null_word should point to some word object which is used by this buffer to represent a non-word.
		Buffer(void* internal_buffer, size_t buffer_size,size_t maximum_single_allocation,word* null_word);

		//Add a word at the end of the doubly linked list
		void add_word(uint8_t* word_location);
		//Add a non-word at the end of the doubly linked list.
		void add_null_word();

		//Set the start of the doubly linked list to be at nmeb bytes.
		void advance_to(size_t nmeb);
		//Set the word at which the buffer's internal pointer is at to be the head of the linked list.
		void set_current_word_as_last_word();

		//Add a word at the start of the doubly linked list. Note that advance_to needs to be called
		//with a suitable value before this function is called, or else what happens is undefined.
		void add_word_at_start(const uint8_t* word_location);
		//Adds a non-word at the start of the doubly linked list.
		void add_null_word_at_start();

		//Allocate numbytes of bytes for a string.
		uint8_t* allocate_for_string(size_t numbytes);
		//Set back the number of bytes taken by the last allocation by numbytes from the end of the region.
		void rewind_string_allocation(size_t numbytes);

		//Returns a pointer to the start of the strings section of the buffer.
		uint8_t* strings_start(void);

		//Set how many words should not be starting words at the start of the doubly linked words list
		void set_top_pointer( size_t nmeb);
		//Set how many words should not be staarting words at the end of the doubly linked words list.
		void set_bottom_pointer( size_t nmeb);

		//Returns a pointer to the first word that should not be a starting word at the end of the words list.
		word* words_top();
		//Returns a pointer to the first word that should be a starting word at at the start of the words-list.
		word* words_bottom();

		//Returns a pointer just past the top of the words list.
		word* words_buffer_top();
		//Returns a pointer to the first word in the words list.
		word* words_buffer_bottom();
		//Write this buffer to disk.
		void writeToDisk();
		bool is_full;
	private:
		void* internal_buffer;
		size_t buffer_size;
		size_t words_bottom_internal;
		size_t strings_top;
		size_t maximum_single_allocation;
		word* last_word;
		word* null_word;

		//Points to the first word that should be counted.
		size_t top_pointer;
		//Points to the first invalid word that should be counted.
		size_t bottom_pointer;

};

struct schedule_entry
{
	off_t start;
	off_t end;
	unsigned int buffercount;
};

unsigned long long int number_of_special_combinations(unsigned int number);

class IndexCollection
{
	public:
		IndexCollection(unsigned int ngramsize,bool build_all_wordsearch_indexes,bool is_pos_supplement_index,int single_wordsearch_index_to_build);
		~IndexCollection();
		void writeBufferToDisk(unsigned int buffercount,unsigned int rightmost_run,Buffer* buffer_to_write,word* null_word);
		void writeMetadata(Metadata &metadata_file);
		void copyToFinalPlace(int k);

		void add_range_to_schedule(off_t start, off_t end);
		bool mark_the_fact_that_a_range_has_been_read_in();
		void writeEmptyLastBuffer(int buffercount);
		pair<schedule_entry,int> get_next_schedule_entry();

	private:
		unsigned int ngramsize;

		unsigned int numcombos;
		vector<unsigned int* >combinations;
		vector<optimized_combination> optimized_combinations;
		vector<char*> prefixes;

		uint8_t (*mergeschedulers)[2*MAX_BUFFERS - 1]; //For merging 
		stack<schedule_entry> unread_ranges;
		int num_being_read;
		int buffercounter;
		bool is_pos_supplement_index;

};

#endif
