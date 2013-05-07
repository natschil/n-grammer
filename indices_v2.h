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
	uint32_t contents;
	uint32_t reduces_to;
	uint32_t flags;
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

		void migrate_from(Buffer* prev,int is_last_buffer);

		void add_word(uint8_t* word_location,int &memmgnt_retval );
		void add_null_word();
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

		//Points to the first word that should be counted.
		size_t top_pointer;
		//Points to the first invalid word that should be counted.
		size_t bottom_pointer;

};


class IndexCollection
{
	public:
		IndexCollection(unsigned int buffer_size,size_t maximum_single_allocation,unsigned int ngramsize,unsigned int wordsearch_index_upto);
		~IndexCollection();

		void writeBufferToDisk(unsigned int buffercount,unsigned int rightmost_run,Buffer* buffer_to_write);
		void makeNewBuffer();
		void writeMetadata(FILE* metadata_file);
		void copyToFinalPlace(int k);

		void increment_numbuffers_in_use()
		{
			#pragma omp atomic
			numbuffers_in_use++;

		}
		void decrement_numbuffers_in_use()
		{
			#pragma omp atomic
			numbuffers_in_use--;
		}
		int get_numbuffers_in_use()
		{
			#pragma omp flush
			return numbuffers_in_use;
		}

		void add_word(uint8_t* word_location, int &memmgnt_retval)
		{
			return current_buffer->add_word(word_location,memmgnt_retval);
		};

		void add_null_word()
		{
			return current_buffer->add_null_word();
		}

		const word* get_null_word()
		{
			return &null_word;
		}

		uint8_t* allocate_for_string(size_t numbytes, int &memmngt_retval)
		{
			return current_buffer->allocate_for_string(numbytes,memmngt_retval);
		};
		void rewind_string_allocation(size_t numbytes)
		{
			return current_buffer->rewind_string_allocation(numbytes);
		};
		void set_top_pointer(size_t nmeb)
		{
			return current_buffer->set_top_pointer(nmeb);
		};
		void set_bottom_pointer(size_t nmeb)
		{
			return current_buffer->set_bottom_pointer(nmeb);
		};

		Buffer* current_buffer;
	private:
		size_t buffer_size;
		size_t maximum_single_allocation;
		size_t ngramsize;
		word null_word;

		size_t numcombos;
		vector<unsigned int* >combinations;
		vector<optimized_combination> optimized_combinations;
		vector<char*> prefixes;
		uint8_t (*mergeschedulers)[MAX_K][MAX_BUFFERS]; //For merging 

		int numbuffers_in_use;

};

#endif
