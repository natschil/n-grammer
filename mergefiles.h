#ifndef MERGEFILES_H
#define MERGEFILES_H
#include <stdio.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <sys/stat.h>

//How big our buffer for mergeing files should be
#define MERGE_BUFFER_SIZE (4 * 1024)
//Merges files in_first with in_second in way useful for the ngram counter.
//See merge_files.c for an example program.
//Note that max_string_length serves merely as a hint and that hence merge_files will work
//Even with input files containing lines with strings longer than max_string_length.

//Returns 0 on sucess, non-zero on error.
int merge_files(FILE* in_first, FILE* in_second, FILE* out,int max_string_length);

//This function outputs the location of the output file.
//Only call this when you are sure that all merging has finished
//Returns -1 on error, though this is really an impossibility.
int get_final_k(void);

//Call this function with k,n referring to an n-gram collection that already exists.
//i.e. when the n-th nonterminal buffer has been written to disk, call schedule_next_merge(0,n,0)
//When the n-th buffer is also the last buffer, call schedule_next_merge(0,n,1)
void schedule_next_merge(int k, int n,int rightmost_run);

//Set a value to use as a hint as the buffer size of the buffer passed to getline for the ngram string length.
void init_merger(int new_max_ngram_string_length);

#endif
