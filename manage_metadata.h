/* (c) 2013 Nathanael Schilling
 * This file includes functions for easy manipulation of the metadata files
 * generated by ngram.counting
 */
#ifndef NGRAM_ANALYIS_METADATA_MANAGEMENT_H
#define NGRAM_ANALYIS_METADATA_MANAGEMENT_H

#include <set>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string.h>
#include <algorithm>

using namespace std;
class Metadata
{
	public:
		Metadata(){};//Do not use
		Metadata(string &filename,string &foldername);
		void write();

		set<vector<unsigned int> > indices;
		bool posIndexesExist;
		set<vector<unsigned int> > inverted_indices;
		bool wordlength_stats_exist;

		unsigned long long num_words;
		unsigned int time_taken;
		unsigned int max_frequency;
		string file_name;
		string metadata_filename;
		string folder_name;

		int MAX_WORD_SIZE;
		int MAX_CLASSIFICATION_SIZE;
		int MAX_LEMMA_SIZE;
};


#endif
