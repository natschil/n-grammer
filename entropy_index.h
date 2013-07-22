/* (c) 2013 Nathanael Schilling
 * This function contains files useful for searching for various n-gram combinations.
 */

#ifndef NGRAM_ANALYSIS_ENTROPY_INDEX_H
#define NGRAM_ANALYSIS_ENTROPY_INDEX_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <math.h>

#include <map>
#include <vector>
#include <string>
#include <sstream>
#include <functional>

#include "manage_metadata.h"
#include "util.h"

using namespace std;
void entropy_index( map<unsigned int,Metadata> &metadatas,vector<string> arguments);

#endif
