//(c) 2013 Nathanael Schilling
#ifndef NGRAM_COUNTING_DICT_H
#define NGRAM_COUNTING_DICT_H

#include <algorithm>
#include <sstream>

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unistdio.h>

#include "searchindexcombinations.h"
#include "words.h"


class Dict;
class DictCollection;


class DictCollection
{
	public:
		DictCollection(
				unsigned int ngramsize,
				unsigned int buffercount,
				vector<unsigned int*> &combinations,
				vector<optimized_combination> &optimized_combinations,
				vector<char*> &prefixes,
				const word* null_word,
				word* buffer_bottom,
				uint8_t* strings_start
				);
		void writeToDisk(word* start);
		void cleanUp(void);
	private:
		unsigned int ngramsize;
		unsigned int buffercount;
		unsigned int numcombos;
		vector<Dict> dictionaries;
};


class Dict
{
    public:
	Dict(
		unsigned int ngramsize,
		const char* prefix,
	       	unsigned int buffercount,
		const optimized_combination *optimized_combo,
		const unsigned int* word_order,
		const word* null_word,
		word* buffer_bottom,
		uint8_t* strings_start
		);
	void cleanUp(void);
	void writeToDisk(word* start);
	void writeOutNGram(word* ngram_start,long long int count);
    private:
	const optimized_combination *optimized_combo;
	const unsigned int* word_order;

	ngram_cmp cmp;
	unsigned int ngramsize;
	unsigned int buffercount;
	const char* prefix;
	const word* null_word;
	word* buffer_bottom;
	uint8_t* strings_start;
	FILE* outfile;
};

#endif
