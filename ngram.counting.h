#ifndef NGRAM_COUNTER_H
#define NGRAM_COUNTER_H
// Count word n-grams in a corpus.
//
#include <deque>

#include <stdio.h>
#include <locale.h>
#include <wchar.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/time.h>

#include <omp.h>

#include <unistr.h>
#include <uninorm.h>
#include <unitypes.h>
#include <unictype.h>
#include <unistdio.h>
#include <unicase.h>
#include <ftw.h>

//We include our configuration options
#include "config.h"



#include "memory_management.h"
#include "mergefiles.h"
#include "indices.h"

long long int count_ngrams(unsigned int ngramsize,FILE* infile,const char* outdir,unsigned int wordsearch_index_depth);
#endif
