//(c) 2013 Nathanael Schilling
#ifndef MERGEFILES_H
#define MERGEFILES_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include "config.h"
/*This file contains functions useful for merging two files where all lines are of the following form:
 * <string><tab><number><newline>
 * and are sorted lexographically by string. 
 *
 * The output file is a merge of the two previous files, so that for lines with equal strings the numbers are added. 
 *
 * The schedule_next_merge function provides a method of sorting more than two files using a 
 */


//Set a value to use as a hint as the buffer size of the buffer passed to getline for the ngram string length.
void init_merger(int new_max_ngram_string_length);

//Call this function with k,n referring to an n-gram collection that already exists.
//i.e. when the n-th non-last buffer has been written to disk, call schedule_next_merge(0,n,0)
//When the n-th buffer is also the last buffer, call schedule_next_merge(0,n,1)
void schedule_next_merge(int k, int n,int rightmost_run,uint8_t (*scheduling_table)[MAX_K][MAX_BUFFERS],const char* prefix);

//Merges files in_first with in_second in way useful for the ngram counter.
//See merge_files.c for an example program.
//Note that max_string_length serves merely as a hint and that hence merge_files will work
//Even with input files containing lines with strings longer than max_string_length.

//Returns 0 on sucess, non-zero on error.
int merge_files(FILE* in_first,off_t first_size, FILE* in_second,off_t second_size, FILE* out,int max_string_length);

//This function outputs the location of the output file.
//Only call this when you are sure that all merging has finished
//Returns -1 on error, though this is really an impossibility.
int get_final_k(void);

#ifdef __cplusplus
}
#endif
#endif
