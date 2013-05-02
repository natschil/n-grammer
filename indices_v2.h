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
#include <map>
#include <vector>
#include <algorithm>

#include "config.h"
#include "searchindexcombinations.h"
#include "mergefiles.h"
#include "util.h"
//#include "qsort.h"

//TODO: turn this into something meaningful
#define cplusplus11

class word
{

	public:
	word(){};
	#ifdef cplusplus11
	word(word&& old) //move constructor
	{
		prev = old.prev;
		next = old.next;
		prev->next = this;
		next->prev = this;	
		this->contents = old.contents;
		this->next = old.next;
		this->prev = old.prev;
	};

	word& operator=(word&& old)
	{			
		prev = old.prev;
		next = old.next;
		prev->next = this;
		next->prev = this;	
		this->contents = old.contents;
		this->next = old.next;
		this->prev = old.prev;
		return *this;
	}

	#endif
	word* prev;
	const uint8_t* contents;
	word* next;
	bool operator<(const word &second) const
	{
		return strcmp((const char*) this->contents,(const char*) second.contents) < 0;	
	}
};
#ifndef cplusplus11
//TODO: Reenable this once everything else works..
namespace std
{
	template<>
	void swap(word& left, word& right)
	{
		left.prev->next = &right;
		left.next->prev = &right;

		right.prev->next = &left;
		right.next->prev = &left;

		const uint8_t* tmp = left.contents;
		left.contents = right.contents;
		right.contents = tmp;

		word* tmp_word_ptr = left.prev;
		left.prev = right.prev;
		right.prev = tmp_word_ptr;

		tmp_word_ptr = left.next;
		left.next = right.next;
		right.next = tmp_word_ptr;
	}

}
#endif
struct NGram
{
	vector<const uint8_t*> ngram;
};
struct ngram_cmp : public std::binary_function<NGram, NGram,bool>
{
	ngram_cmp(){};//To make stl map happy
	ngram_cmp(unsigned int,const unsigned int*);
	bool operator()(NGram first, NGram second);
	private:
	unsigned int n_gram_size;
	const unsigned int *word_order;
};

class Buffer
{
	public:
		Buffer(void* internal_buffer, size_t buffer_size,size_t maximum_single_allocation,word* null_word);
		~Buffer();
		void add_word(uint8_t* word_location,int &memmgnt_retval );
		void add_null_word(word* null_word);
		uint8_t* allocate_for_string(size_t numbytes, int &memmngnt_retval);
		void rewind_string_allocation(size_t numbytes);
		void set_starting_pointer( size_t nmeb);
		void set_ending_pointer( size_t nmeb);
		word* words_begin();
		word* words_end();
		word* words_buffer_begin();
		word* words_buffer_end();
		void writeToDisk();
	private:
		void* internal_buffer;
		size_t buffer_size;
		size_t words_bottom;
		size_t strings_top;
		size_t maximum_single_allocation;
		word* last_word;

		//Points to the first word that should be counted.
		size_t starting_pointer;
		//Points to the first invalid word that should be counted.
		size_t ending_pointer;

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
			return numbuffers_in_use;
		}

		void add_word(uint8_t* word_location, int &memmgnt_retval)
		{
			return current_buffer->add_word(word_location,memmgnt_retval);
		};

		void add_null_word()
		{
			return current_buffer->add_null_word(&null_word);
		}

		uint8_t* allocate_for_string(size_t numbytes, int &memmngt_retval)
		{
			return current_buffer->allocate_for_string(numbytes,memmngt_retval);
		};
		void rewind_string_allocation(size_t numbytes)
		{
			return current_buffer->rewind_string_allocation(numbytes);
		};
		void set_starting_pointer(size_t nmeb)
		{
			return current_buffer->set_starting_pointer(nmeb);
		};
		void set_ending_pointer(size_t nmeb)
		{
			return current_buffer->set_ending_pointer(nmeb);
		};

		Buffer* current_buffer;
	private:
		size_t buffer_size;
		size_t maximum_single_allocation;
		size_t ngramsize;
		word null_word;

		size_t numcombos;
		vector<unsigned int* >combinations;
		vector<char*> prefixes;
		uint8_t (*mergeschedulers)[MAX_K][MAX_BUFFERS]; //For merging 

		int numbuffers_in_use;

};

#endif
