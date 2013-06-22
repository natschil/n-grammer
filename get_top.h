/* (c) 2013 Nathanael Schilling
 * This file contains functions for getting data from the inverted indeces generated invert_index.h
 */
#ifndef SEARCH_INVERTED_INDEX
#define SEARCH_INVERTED_INDEX

#include <vector>
#include <map>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "manage_metadata.h"


void get_top(map<unsigned int,Metadata> &metadatas,vector<string> &arguments);


#endif
