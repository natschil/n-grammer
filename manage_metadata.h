/* (c) 2013 Nathanael Schilling
 * This file provides an abstraction for the format used by both the n-gram counter
 * and the n-gram analysis programs to store metadata information.
 * It provides the Metadata class which can be modified and then subsequently written to disk.
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


/*
 * To prevent problems from arising having different (e.g. either obsolete or too new) expectations of the format of the output
 * of the indexes created the by the n-gram counter, we stamp the metadata with a version number at the top. This also very nicely
 * allows us to identify it as valid n-gram counter metadata. 
 */
#ifndef NGRAM_COUNTER_OUTPUT_FORMAT_VERSION
	#define NGRAM_COUNTER_OUTPUT_FORMAT_VERSION 2
#endif

using namespace std;

class Metadata
{
	public:
		/*
		 * The constructor function.
		 * <foldername>/<filename> should point to the location of the metadata file.
		 * If <fileMayNotExist> is set, an error is thrown when an invalid file or nonexistent file is encountered.
		 * Otherwise, this constructor returns silently, though setting fileExistsAlready to false.
		 */
		Metadata(string &metadata_filename);
		/*
		 * This function writes the fields of the (possibly updated) Metadata struct to a file in the format in which it reads the
		 * file. Note that it does so even if the file did not previously exist.
		 */
		void write();

	/*The following fields are set by the n-gram counter program, or are common to both programs*/

		//The filename of the corpus analyzed, possibly in relative form.
		string file_name;
		//The name of the folder containing all of the actual data (and usually also metadata).
		string output_folder_name;
		//How many words the corpus contained.
		long long num_words;
		//Whether or not the file had part of speech markup present.
		bool is_pos;
		//The total time taken by the n-gram counter to process the corpus.
		int time_taken;
		//See MAX_WORD_SIZE,MAX_CLASSIFICATION_SIZE and MAX_LEMMA_SIZE in n-gram counter's config.h
		int max_word_size;
		int max_classification_size;
		int max_lemma_size;

		//The indexes that exist for this particular value of 'n'.
		set<vector<unsigned int> > indices;
		//Whether or not POS supplement indexes exist.
		bool pos_supplement_indexes_exist;

		//The path to the file that this object represents.
		string metadata_filename;
		//Whether or not the file that this represents already exists on disk or not. 
		//This field is set to false if an error occurs when reading a metadata file.
		bool file_exists_already;

	/*The following fields are only set by the n-gram analysis program*/	

		//The set of inverted indexes, by frequency of occurance.
		set<vector<unsigned int> > inverted_indices;
		//The maximal frequency found in the database
		unsigned int max_frequency;

		//Whether or not wordlength statistics exist.
		bool wordlength_stats_exist;


};


#endif
