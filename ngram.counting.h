#ifndef NGRAM_COUNTER_H
#define NGRAM_COUNTER_H
// Count word n-grams in a corpus.
//

#include <stdio.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#include <sys/stat.h>
#include <sys/time.h>

#include <omp.h>

#include <unistr.h>
#include <uninorm.h>
#include <unitypes.h>
#include <unictype.h>
#include <unistdio.h>
#include <unicase.h>

//We include our configuration options
#include "config.h"

#include "mergefiles.h"
#include "indices_v2.h"

long long int count_ngrams(unsigned int ngramsize,const char* infile_name,const char* outdir,unsigned int wordsearch_index_depth,unsigned int num_concurrent_buffers,bool cache_entire_file,bool is_pos,bool build_pos_supplement_indexes,bool build_smaller_indexes);
#endif
