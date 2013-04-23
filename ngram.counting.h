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
#include <sys/time.h>
#include <errno.h>

#include <omp.h>

#include <unistr.h>
#include <unistdio.h>
#include <uninorm.h>
#include <unitypes.h>
#include <unictype.h>
#include <unistdio.h>
#include <unicase.h>

//We include our configuration options
#include "config.h"



#include "memory_management.h"
#include "mergefiles.h"
#include "searchindexcombinations.h"

struct myUString
{
	const uint8_t* string;
	size_t length;
	~myUString(){return;};
};

struct myNGram
{
	~myNGram(){return;};
	int num_occurances;
	myUString ngram[0];//See "flexible array members" for why this hack (arguable) works.
};
struct randomstruct
{
	char something;
};


struct ngram_cmp : public std::binary_function<myNGram, myNGram,bool>
{
	bool operator()(myNGram *first, myNGram *second);
};


//We do not actually use the char
typedef std::map<myNGram*,char,ngram_cmp> letterDict;

//Use one "Dictionary" per letter.
typedef letterDict Dict[256];

long long int analyze_ngrams(unsigned int ngramsize,FILE* infile,const char* outdir);
#endif
