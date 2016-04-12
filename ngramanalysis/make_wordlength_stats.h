/*(c) Nathanael Schilling 2013
 * This file contains functions for efficiently building "inverted indexes" from
 * the output files of ngram.counting
 */
#ifndef NGRAM_ANALYIS_WORDLENGTH_STATS_H
#define NGRAM_ANALYIS_WORDLENGTH_STATS_H


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
#include "util.h"

void make_wordlength_stats(map<unsigned int,Metadata> &metadatas,vector<string> &arguments);


#endif
