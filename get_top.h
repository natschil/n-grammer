/* (c) 2013 Nathanael Schilling
 * This file contains functions for getting data from the inverted indeces generated invert_index.h
 */
#ifndef SEARCH_INVERTED_INDEX
#define SEARCH_INVERTED_INDEX

#include <vector>
#include <map>
#include <functional>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "manage_metadata.h"
#include "util.h"


pair<vector<string>,long long int>  getNGramAtAddress(off_t address, void* mmaped_file, off_t mmaped_file_size);
void get_top(map<unsigned int,Metadata> &metadatas,vector<string> &arguments);


#endif
