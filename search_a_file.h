//(c) 2013 Nathanael Schilling


#ifndef NGRAM_ANALYSIS_SEARCH_A_FILE_H
#define NGRAM_ANALYSIS_SEARCH_A_FILE_H
/* This function provides useful functions for binary searching ngram indexes.
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <stdint.h>

#include<map>
#include<string>

#include "manage_metadata.h"

using namespace std;

class searchable_file
{
	public:
		searchable_file(string &filename);
		~searchable_file();
		void search(string &search_str,vector<string> &results); 

	private:
		int fd;
		char* mmaped_index;
		int64_t mmaped_index_size;
		map<string, off_t> search_tree;
};



#endif
