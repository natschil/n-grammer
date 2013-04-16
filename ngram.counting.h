#ifndef NGRAM_COUNTER_H
#define NGRAM_COUNTER_H
// Count word n-grams in a corpus.
//
#include <vector>
#include <map>
#include <deque>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <omp.h>

//#define U_CHARSET_IS_UTF8 1
#include <unicode/utf.h>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/unorm2.h>
#include <unicode/ustdio.h>



#include "memory_management.h"
extern "C"{
#include "mergefiles.h"
}


struct myNGram
{
	UChar **ngram;	
	~myNGram();
};
struct myUString
{
	UChar* string;
	size_t length;
	~myUString(){return;};
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

int getnextword(UChar* &s, UFILE* f,const UNormalizer2* normalizer);

long long int analyze_ngrams(unsigned int ngramsize,FILE* infile,FILE* outfile);
#endif
