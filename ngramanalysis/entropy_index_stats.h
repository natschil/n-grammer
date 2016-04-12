/*
 * (c) 2014 Nathanael Schilling.
 * This file contains declarations of some function used when looking at the
 * distribution of entropy in the entropy-indexes.
 */
#ifndef ENTROPY_INDEX_STATS_H
#define ENTROPY_INDEX_STATS_H

#include <vector>
#include <map>
#include <cmath>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>


#include "manage_metadata.h"
#include "get_top.h"
#include "util.h"



void  entropy_index_stats(map<unsigned int, Metadata> &metadatas, vector<string> arguments);


#endif
 
