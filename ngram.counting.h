#ifndef NGRAM_COUNTER_H
#define NGRAM_COUNTER_H
// Count word n-grams in a corpus.
//
#include <vector>
#include <map>
#include <deque>

#include <stdio.h>
#include <locale.h>
#include <wchar.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include <omp.h>

#include <unistr.h>
#include <unistdio.h>
#include <uninorm.h>
#include <unitypes.h>
#include <unictype.h>
#include <unistdio.h>
#include <unicase.h>

#include "config.h"



#include "memory_management.h"
extern "C"{
#include "mergefiles.h"
}

struct myUString
{
	uint8_t* string;
	size_t length;
	~myUString(){return;};
};

struct myNGram
{
	myUString *ngram;	
	~myNGram();
};


struct ngram_cmp : public std::binary_function<myNGram, myNGram,bool>
{
	bool operator()(myNGram first, myNGram second);
};


typedef std::map<myNGram, long long int,ngram_cmp> letterDict;
//Use one "Dictionary" per letter.
//This creates a somewhat strange sorting but because all we need is for the data to be sorted that is fine.
typedef letterDict Dict[256];
extern letterDict *lexicon;

int getnextword(uint8_t* &s, FILE* f, uninorm_t);

long long int analyze_ngrams(unsigned int ngramsize,FILE* infile,FILE* outfile);
#endif
