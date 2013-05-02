/*(c) 2013 Nathanael Schilling
 * This file contains functions for keeping track of words, n-grams and indices
 */

using namespace std;

#ifndef NGRAM_COUNTING_INDICES_V2_HEADER
#define NGRAM_COUNTING_INDICES_V2_HEADER

//We provide a swap template so that the pointers in the doubly linked list still work even after sorting.

#include <string.h>
#include <stdio.h>
#include <unistr.h>
#include <unistdio.h>
#include <sys/stat.h>
#include <errno.h>


#include <iostream>
#include <map>




namespace std
{
	template<> void swap(word& left, word& right)
	{
		left.prev->next = &right;
		left.next->prev = &right;

		right.prev->next = &left;
		right.next->prev = &left;

		uint8_t* tmp = left.contents;
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

class word
{
	public:
	word* prev;
	const uint8_t* contents;
	word* next;
	bool operator<(word &second)
	{
		return strcmp(this->contents, second->contents) < 0;	
	}
}
class NGram
{
	vector<uint8_t*> ngram;
}
struct ngram_cmp : public std::binary_function<NGram, NGram,bool>
{
	ngram_cmp(unsigned int,const unsigned int*);
	bool operator()(NGram first, NGram second);
	private:
	unsigned int n_gram_size;
	const unsigned int *word_order;
};

class Buffer
{
	public:
		Buffer(void* internal_buffer, size_t buffer_size,size_t maximum_single_allocation,word* null_word)
		{
			this->internal_buffer = internal_buffer;
			this->buffer_size = buffer_size;
			this->words_bottom = 0;
			this->strings_top = buffer_size;
			this->maximum_single_allocation = maximum_single_allocation;
			this->last_word = null_word;
			this->null_word = null_word;
		};
		void add_word(uint8_t* word_location,int &memmgnt_retval );
		uint8_t* allocate_for_string(size_t numbytes, int &memmngnt_retval);
		void rewind_string_allocation(size_t numbytes);
		void set_starting_pointer( size_t nmeb);
		void set_ending_pointer( size_t nmeb);
		word* words_begin(){ return (word*)(internal_buffer + starting_pointer);};
		word* words_end(){ return (word*)(internal_buffer + ending_pointer);};
		void writeToDisk();
	private:
		void* internal_buffer;
		size_t buffer_size;
		size_t words_bottom;
		size_t strings_top;
		size_t maximum_single_allocation;
		word* last_word;
		word* null_word;

		//Points to the first word that should be counted.
		size_t starting_pointer;
		//Points to the first invalid word that should be counted.
		size_t ending_pointer;

};


class IndexCollection
{
	public:
		IndexCollection(size_t buffer_size,size_t maximum_single_allocation);
		~IndexCollection();

		void writeBufferToDisk(unsigned int buffercount,unsigned int rightmost_run,Buffer* buffer_to_write);
		void writeMetadata(FILE* metadata_file);
		void IndexCollection::copyToFinalPlace(int k);


		void add_word(uint8_t* word_location, int &memmgnt_retval)
		{
			return current_buffer->add_word(word_location,memmgnt_retval);
		};

		void add_null_word(int &memmgnt_retval)
		{
			return current_buffer->add_word(&null_word,memmgnt_retval);
		}

		uint8_t* allocate_for_string(size_t numbytes, int &memmngt_retval)
		{
			return current_buffer->allocate_for_string(numbytes,memmngt_retval);
		};
		void rewind_string_allocation(size_t numbytes);
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
		char** prefixes;
		uint8_t (*mergeschedulers)[MAX_K][MAX_BUFFERS]>; //For merging 
}

#endif
