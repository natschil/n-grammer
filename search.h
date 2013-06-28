/* (c) 2013 Nathanael Schilling
 * This function contains files useful for searching for various n-gram combinations.
 */

#ifndef NGRAM_ANALYSIS_SEARCH_H
#define NGRAM_ANALYSIS_SEARCH_H

#include <map>
#include <vector>
#include <string>
#include <sstream>

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <stdint.h>

#include <unistr.h>
#include <uninorm.h>
#include <unitypes.h>
#include <unictype.h>
#include <unistdio.h>
#include <unicase.h>


#include "manage_metadata.h"
#include "searchable_file.h"

using namespace std;
void do_search( map<unsigned int,Metadata> &metadatas,vector<string> arguments);

vector<pair<vector<string>, long long int> > internal_search(map<unsigned int, Metadata> &metadatas,vector<string> arguments);
#endif
