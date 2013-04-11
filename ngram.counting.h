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

//#define U_CHARSET_IS_UTF8 1
#include <unicode/utf.h>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/unorm2.h>
#include <unicode/ustdio.h>



#include "memory_management.h"


struct myUString
{
	size_t length;	
	UChar* str;	
	~myUString();
};

struct uchar_cmp : public std::binary_function<myUString, myUString,bool>
{
	bool operator()(myUString first, myUString second);
};


typedef std::map<myUString, double,uchar_cmp> letterDict;
//Use one "Dictionary" per letter.
//This creates a somewhat strange sorting but because all we need is for the data to be sorted that is fine.
typedef letterDict Dict[256];
extern letterDict *lexicon;

typedef std::deque<myUString> word_list;


int getnextword(UChar* &s, UFILE* f,const UNormalizer2* normalizer);

double analyze_ngrams(unsigned int ngramsize,FILE* file);
#endif
