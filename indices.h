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
		Index(unsigned int ngramsize,vector<unsigned int> &combination);
		int mark_ngram_occurance(NGram*);
		void writeToDisk(int buffercount,int isfinalbuffer);
		void copyToFinalPlace(int k);
	private:
		vector<letterDict> letters;
		unsigned int n_gram_size;
		
		//Word_order specifies which order the words should be in
		vector<unsigned int> word_order;
		uint8_t scheduling_table[MAX_K][MAX_BUFFERS]; //For merging 
		char* prefix;
};
class IndexCollection
{
	public:
		IndexCollection(){};//Do not call
		IndexCollection(unsigned int ngramsize,unsigned int wordsearch_index_upto);
	  	void mark_ngram_occurance(NGram* new_ngram);
	  	void writeToDisk(int buffercount,int isfinalbuffer);
		void copyToFinalPlace(int k);
  private:
	  vector<Index> indices;
	  size_t indices_size;

 	  unsigned int n_gram_size;
};

class IndexCollectionBufferPair
{
  public:
	  IndexCollectionBufferPair(unsigned int ngramsize,unsigned int wordsearch_index_upto);
	  void swapBuffers(void);
	  void mark_ngram_occurance(NGram* new_ngram);
	  void writeBufferToDisk(size_t buffer_num,size_t buffercount,int isfinalbuffer);
	  size_t get_current_buffer(){return current_buffer;}
	  void copyToFinalPlace(int k);
  private:		
	  size_t n_gram_size;
	  size_t current_buffer;
	  IndexCollection buffers[2];
};

#endif
