// Count word n-grams in a corpus.
//
#include <vector>
#include <map>
#include <deque>
#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define U_CHARSET_IS_UTF8 1
#include <unicode/utf.h>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/normalizer2.h>
#include <unicode/ustdio.h>


typedef std::map<icu::UnicodeString, double> Dict;
typedef std::deque<icu::UnicodeString> word_list;


UnicodeString getnextword(UFILE* f,int &over);

void writeDict(Dict& D, const double corpussize);

Dict &mark_ngram_occurance(Dict &lexicon, UnicodeString new_ngram);
double analyze_ngrams(Dict &lexicon,unsigned int ngramsize,FILE* file);
