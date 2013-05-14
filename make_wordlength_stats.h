/*(c) Nathanael Schilling 2013
 * This file contains functions for efficiently building "inverted indexes" from
 * the output files of ngram.counting
 */
#ifndef NGRAM_ANALYIS_WORDLENGTH_STATS_H
#define NGRAM_ANALYIS_WORDLENGTH_STATS_H

//TODO: Get MAX_WORD_SIZE using the metadata files.
#ifndef MAX_WORD_SIZE
#define MAX_WORD_SIZE 40
#endif

#include <string>
#include <vector>
#include <map>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <dirent.h>
#include <fcntl.h>
#include "manage_metadata.h"

void make_wordlength_stats(map<unsigned int,Metadata> &metadatas,vector<string> &arguments);


#endif
