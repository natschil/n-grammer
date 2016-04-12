/* (c) 2013 Nathanael Schilling
 * This File defines various utils
 */
#ifndef NGRAM_COUNTER_UTILS_H
#define NGRAM_COUNTER_UTILS_H

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 500
#endif
#include <ftw.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

/* This function does approximately what rm -r does, but only for directories.
 */
void recursively_remove_directory(const char* directory_name);

#ifdef __cplusplus
}
#endif

#endif
