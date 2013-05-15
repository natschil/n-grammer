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

/*
 * The word class is an element of a doubly linked list, with next and prev pointing to the next word and previous words in the text respectively.
 * contents points to a nul-terminated uint8_t* string with the actual contents of the word.
 * reduces_to is an additional link that is useful in contexts where we have a sorted array of words and want to compare two of them. By
 * setting reduces_to the highest element of the list that the word is equal to, to compare words it is sufficient to compare their
 * reduces_to  members. This optimization saves time in exchange for additional memory usage.
 * Currently, "flags" simply indicates whether the word is a possible starting word of an n-gram. This is not always the case
 * due to the way that buffers overlap.
 */

#define NON_STARTING_WORD 1

class word
{
    public:
	word* prev;
	uint64_t contents;
	uint64_t reduces_to;
	uint8_t flags;
	word* next;
 	//The following functions are required so the structure of the doubly linked list is preserved throughout sorting.
	word(){};
	//move constructor
	word(word&& old);
	//move operator=
	word& operator=(word &&old);
};

/* The word_cmp class is used for comparing two words without using the reduces_to field
 */
struct word_cmp
{
	word_cmp();
	//The parameter strings_start should point to the value that the contents field of the word class is relative to.
	word_cmp(const char* strings_start);
	bool operator()(const word &first, const word &second);
	private:
	const char* strings_start;
};

/* The ngram_cmp class is used for comparing n-grams that are chains of words in a doubly linked list. It uses the reduces_to
 * field to compare individual words
 */
struct ngram_cmp 
{
	ngram_cmp(){};//Don't use this, it merely exists for various standard functions not to fail.
	ngram_cmp(unsigned int ngramsize,const unsigned int* word_combination_order,const optimized_combination* optimized_combo,const word* null_word);
	bool operator()(const word &first,const word &second);
	private:
	unsigned int n_gram_size;
	const unsigned int *word_order;
	const optimized_combination* optimized_combo;
	const word* null_word;
};

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

class IndexCollection
{
	public:
		IndexCollection(unsigned int ngramsize,unsigned int wordsearch_index_upto,bool is_pos_supplement_index = false);
		~IndexCollection();
		void writeBufferToDisk(unsigned int buffercount,unsigned int rightmost_run,Buffer* buffer_to_write,word* null_word);
		void writeMetadata(FILE* metadata_file);
		void copyToFinalPlace(int k);

		void add_range_to_schedule(off_t start, off_t end);
		bool mark_the_fact_that_a_range_has_been_read_in();
		void writeEmptyLastBuffer(int buffercount);
		pair<schedule_entry,int> get_next_schedule_entry();

	private:
		size_t ngramsize;

		size_t numcombos;
		vector<unsigned int* >combinations;
		vector<optimized_combination> optimized_combinations;
		vector<char*> prefixes;

		uint8_t (*mergeschedulers)[MAX_K][MAX_BUFFERS]; //For merging 
		stack<schedule_entry> unread_ranges;
		int num_being_read;
		int buffercounter;
		bool is_pos_supplement_index;

};

#endif
