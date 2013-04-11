#ifndef MERGEFILES_H
#define MERGEFILES_H
#include <stdio.h>
#include <limits.h>

//Merges files in_first with in_second in way useful for the ngram counter.
//See merge_files.c for an example program.
//Note that max_string_length serves merely as a hint and that hence merge_files will work
//Even with input files containing lines with strings longer than max_string_length.

//Returns 0 on sucess, non-zero on error.
int merge_files(FILE* in_first, FILE* in_second, FILE* out,int max_string_length);

#endif
