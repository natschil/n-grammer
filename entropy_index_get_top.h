/* (c) 2013 Nathanael Schilling
 * This file contains functions for getting data from the inverted indeces generated invert_index.h
 */
#ifndef ENTROPY_INDEX_GET_TOP_H
#define ENTROPY_INDEX_GET_TOP_H

#include <vector>
#include <map>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "manage_metadata.h"
#include "get_top.h"
#include "util.h"


void entropy_index_get_top(map<unsigned int,Metadata> &metadatas,vector<string> &arguments);


#endif
