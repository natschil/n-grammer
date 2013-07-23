//(c) 2013 Nathanael Schilling


#ifndef NGRAM_ANALYSIS_SEARCH_A_FILE_H
#define NGRAM_ANALYSIS_SEARCH_A_FILE_H
/* This function provides an abstraction level for easily binary searching ngram indexes.
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
#include <algorithm>
#include <cctype>

#include "manage_metadata.h"

using namespace std;

class searchable_file
{
	public:
		searchable_file(const string &filename,bool is_pos=false);
		~searchable_file();
		searchable_file(const searchable_file &old) = delete;
		searchable_file operator=(const searchable_file &old) = delete;

		/* This function does a binary search on the file.
		 * search_str should be the part of the string actually in the file (including the final tab).
		 * filter should be the re-ordered search-string (including wildcards) that any string should match in the file.
		 */
		void search(const string &search_str,vector<string> &results,const string &filter); 

	private:
		bool string_matches_filter(const string &result_s,const string &filter_s,const string &search_string_s);

		string index_filename;
		int fd;
		char* mmaped_index;
		int64_t mmaped_index_size;
		map<string, int64_t> search_tree;
		bool is_pos;
};



#endif
