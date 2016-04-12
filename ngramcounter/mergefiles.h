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
#include "util.h"
/*This file contains functions useful for merging two files where all lines are of the following form:
 * <string><tab><number><newline>
 * and are sorted using binary comparison by string. 
 *
 * The output file is a merge of the two files,but so that lines with equal strings have their numbers added. 
 *
 * The schedule_next_merge function provides a method of scheduling the merging of multiple files so that the appropriate
 * merges take place as new files have been written out.
 */


//Set a value to use as a hint as the buffer size of the buffer passed to getline for the ngram string length.
void init_merger(int new_max_ngram_string_length);

/*Call this function with the arguments k and n (see below) referring to a file that already exists.
*	k is the merge depth, and is 0 if the file is not the result of any pervious merges.
*	n is this file's unique file number.
* rightmost_run should be set if this file is the file with the largest file number.
* Note that file numbers cannot be sparse, i.e. files 0 to <maximum file number> must eventually exist for all merges to complete, 
* but the order at which they are registered does not matter.
* scheduling table should point to a region of memory unique to this sequence of files.
*
* e.g when the n-th non-last buffer has been written to disk, call schedule_next_merge(0,n,0)
* When the n-th buffer is also the last buffer, call schedule_next_merge(0,n,1)
*/
void schedule_next_merge(int k, int n,int rightmost_run,uint8_t (*scheduling_table)[2*MAX_BUFFERS - 1],const char* prefix);

/*Merges files in_first with in_second in the way described above
* See merge_files.c for an example program using this file
* Note that max_string_length serves merely as a hint and that hence merge_files will work
* even with input files containing lines with strings longer than max_string_length.
* Returns 0 on sucess and non-zero on error.
*/

int merge_files(FILE* in_first,off_t first_size, FILE* in_second,off_t second_size, FILE* out,int max_string_length);

/* This function returns the maximum merge depth encountered by the merge scheduler.
 * For the case of the n-gram counter, this tells us the filename of the file resulting from the final merge.
 * Only call this when you are sure that all merging has finished
 */
int get_final_k(void);

#ifdef __cplusplus
}
#endif
#endif
