/*(c) 2013 Nathanael Schilling
 * This file contains functions for keeping track of words, n-grams and indices
 */

using namespace std;

#ifndef NGRAM_COUNTING_INDICES_V2_HEADER
#define NGRAM_COUNTING_INDICES_V2_HEADER

//We provide a swap template so that the pointers in the doubly linked list still work even after sorting.

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

#include "config.h"
#include "searchindexcombinations.h"
#include "mergefiles.h"
#include "util.h"

/*
 * The word class is an element of a doubly linked list, with next and prev pointing to the next word and previous words in the text respectively.
 * contents points to a null-terminated uint8_t* string with the actual contents of the word.
 * reduces_to is an additional link that is useful in contexts where we have a sorted array of words and want to compare two of them. By
 * setting reduces_to the lowest element of the list that the word is equal to, to compare words it is sufficient to compare their
 * reduces_to class members. This optimization saves time in exchange for additional memory usage.
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
	word(){}; //required by std::sort
	word(word&& old) //move constructor
	{
		prev = old.prev;
		next = old.next;
		contents = old.contents;
		reduces_to = old.reduces_to;
		flags = old.flags;
		prev->next = this;
		next->prev = this;	
		this->next = old.next;
		this->prev = old.prev;

	};

	word& operator=(word&& old)//move operator=
	{			
		prev = old.prev;
		next = old.next;
		flags = old.flags;
		contents = old.contents;
		reduces_to = old.reduces_to;

		prev->next = this;
		next->prev = this;	

		this->next = old.next;
		this->prev = old.prev;
		return *this;
	};
};

struct word_cmp
{
	word_cmp();
	word_cmp(char* strings_start)
	{
		this->strings_start = strings_start;
	}
	bool operator()(const word &first, const word &second)
	{
		return strcmp(strings_start + first.contents, strings_start + second.contents) < 0;
	}
	private:
	char* strings_start;
};

struct ngram_cmp 
{
	ngram_cmp(){};//Don't use this
	ngram_cmp(unsigned int ngramsize,const unsigned int*,const optimized_combination* optimized_combo,const word* null_word);
	bool operator()(const word &first,const word &second);
	private:
	unsigned int n_gram_size;
	const unsigned int *word_order;
	const optimized_combination* optimized_combo;
	const word* null_word;
};

class Buffer
{
	public:
		Buffer(void* internal_buffer, size_t buffer_size,size_t maximum_single_allocation,word* null_word);
		~Buffer();


		void add_word(uint8_t* word_location,int &memmgnt_retval );
		void add_null_word();

		void advance_to(size_t nmeb);
		void set_current_word_as_last_word();

		void add_word_at_start(const uint8_t* word_location);
		void add_null_word_at_start();

		uint8_t* allocate_for_string(size_t numbytes, int &memmngnt_retval);
		uint8_t* strings_start(void);
		void rewind_string_allocation(size_t numbytes);
		void set_top_pointer( size_t nmeb);
		void set_bottom_pointer( size_t nmeb);
		word* words_top();
		word* words_bottom();
		word* words_buffer_top();
		word* words_buffer_bottom();
		void writeToDisk();
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
		IndexCollection(unsigned int ngramsize,unsigned int wordsearch_index_upto);
		~IndexCollection();
		void writeBufferToDisk(unsigned int buffercount,unsigned int rightmost_run,Buffer* buffer_to_write,word* null_word);
		void writeMetadata(FILE* metadata_file);
		void copyToFinalPlace(int k);

		void add_range_to_schedule(off_t start, off_t end);
		void mark_the_fact_that_a_range_has_been_read_in(void);
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

};

#endif
