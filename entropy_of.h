/* (c) 2013 Nathanael Schilling
 * This function contains files useful for searching for various n-gram combinations.
 */

#ifndef NGRAM_ANALYSIS_ENTROPY_OF_H
#define NGRAM_ANALYSIS_ENTROPY_OF_H

#include <map>
#include <vector>
#include <string>
#include <sstream>

#include "search.h"
#include "manage_metadata.h"
#include "math.h"

using namespace std;
void entropy_of( map<unsigned int,Metadata> &metadatas,vector<string> arguments);

#endif
