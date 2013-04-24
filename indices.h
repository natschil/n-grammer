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
	ngram_cmp(){return;};//Do NOT use
	bool operator()(NGram *first, NGram *second);
	private:
	size_t n_gram_size;
};

typedef std::map<NGram*,char,ngram_cmp> letterDict;

class Index
{
	public:
		Index(){};//Do not call.
		Index(size_t ngramsize);
		void mark_ngram_occurance(NGram*);
		void writeToDisk(int buffercount);
	private:
		vector<letterDict> letters;
		size_t n_gram_size;


};
class IndexCollection
{
	public:
		IndexCollection(){};//Do not call
		IndexCollection(size_t ngramsize);
	  	void mark_ngram_occurance(NGram* new_ngram);
	  	void writeToDisk(int buffercount);
  private:
	  vector<Index> indeces;
 	  size_t n_gram_size;
};

class IndexCollectionBufferPair
{
  public:
	  IndexCollectionBufferPair(size_t ngramsize);
	  void swapBuffers(void);
	  void mark_ngram_occurance(NGram* new_ngram);
	  void writeBufferToDisk(size_t buffer_num,size_t buffercount);
	  size_t get_current_buffer(){return current_buffer;}
  private:		
	  size_t n_gram_size;
	  size_t current_buffer;
	  IndexCollection buffers[2];
};

#endif
