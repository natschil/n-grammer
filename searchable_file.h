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

#include "manage_metadata.h"

using namespace std;

class searchable_file
{
	public:
		searchable_file(string &filename,bool is_pos=false);
		~searchable_file();
		searchable_file(const searchable_file &old) = delete;
		searchable_file operator=(const searchable_file &old) = delete;
		/*
		 * <search_str> is the part of the string that is actually contained in the file, <filter> 
		 *
		 */
		void search(string &search_str,vector<string> &results,string &filter); 

	private:
		bool string_matches_filter(string &result_s, string &filter_s,string &search_string_s);

		string index_filename;
		int fd;
		char* mmaped_index;
		int64_t mmaped_index_size;
		map<string, off_t> search_tree;
		bool is_pos;
};



#endif
