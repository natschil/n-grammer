/*(c) 2013 Nathanael Schilling
 * This header file describes the data structures and functions used for keeping track of ngrams as they are added, and writing them to disk
 */
#ifndef NGRAM_COUNTING_CPP_H
#define NGRAM_COUNTING_CPP_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistr.h>
#include <unistdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <errno.h>

#include <map>
#include <vector>

#include "config.h"
#include "mergefiles.h"
#include "util.h"


//We do not actually use the char
using namespace std;

struct myUString
{
	const uint8_t* string;
	size_t length;
};
	
struct NGram
{
	int num_occurances;
	myUString ngram[0];//See "flexible array members" for why this hack (arguable) works.
};

struct ngram_cmp : public std::binary_function<NGram, NGram,bool>
{
	ngram_cmp(size_t ngramsize)
	{
		n_gram_size = ngramsize;
	}
	ngram_cmp(){return;};//So there is a default constructor and things like vector and map are happy.
	ngram_cmp(unsigned int,vector<unsigned int>&); //<-This constructor is fine to use though.
	bool operator()(NGram *first, NGram *second);
	private:
	unsigned int n_gram_size;
	vector<unsigned int> word_order;
};

typedef std::map<NGram*,char,ngram_cmp> letterDict;

class Index
{
	public:
		Index(){};//Do not call.
		Index(unsigned int ngramsize,vector<unsigned int> &combination,const char* prefix);
		int mark_ngram_occurance(NGram*);
		void writeToDisk(int buffercount);
		void copyToFinalPlace(int k);
	private:
		//Word_order specifies which order the words should be in
		vector<letterDict> letters;
		vector<unsigned int> word_order;
		const char* prefix;
		unsigned int n_gram_size;
};

class IndexBufferPair
{
	public:
	      	IndexBufferPair(){}; //Do not call
		IndexBufferPair(unsigned int ngramsize, vector<unsigned int> &combination);
		int mark_ngram_occurance(NGram*);
		void writeBufferToDisk(int buffer_num, int buffercount, int isfinalbuffer);
		void copyToFinalPlace(int k);
		void swapBuffers();
		int getCurrentBuffer(void);
	private:
		Index buffers[2];	
		size_t current_buffer;
		uint8_t scheduling_table[MAX_K][MAX_BUFFERS]; //For merging 
		char* prefix;
		vector<unsigned int> word_order;
};

class IndexCollection
{
	public:
		IndexCollection(){};//Do not call.
		IndexCollection(unsigned int ngramsize, unsigned int word_search_index_depth);
		void mark_ngram_occurance(NGram*);
		void writeBufferToDisk(int buffer_num, int buffercount, int isfinalbuffer);
		void copyToFinalPlace(int k);
		void swapBuffers(void);
		int getCurrentBuffer(void);
	private:
		vector<IndexBufferPair> indices;
		size_t indices_size;

		unsigned int n_gram_size;
};

#endif
