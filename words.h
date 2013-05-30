#ifndef NGRAM_COUNTER_WORDS_H
#define NGRAM_COUNTER_WORDS_H

#include <stdlib.h>
#include "searchindexcombinations.h"


/*
 * The word class is an element of a doubly linked list, with next and prev pointing to the next word and previous words in the text respectively.
 * contents points to a nul-terminated uint8_t* string with the actual contents of the word.
 * reduces_to is an additional link that is useful in contexts where we have a sorted array of words and want to compare two of them. By
 * setting reduces_to the highest element of the list that the word is equal to, to compare words it is sufficient to compare their
 * reduces_to  members. This optimization saves time in exchange for additional memory usage.
 * Currently, "flags" simply indicates whether the word is a possible starting word of an n-gram. This is not always the case
 * due to the way that buffers overlap.
 */

#define NON_STARTING_WORD 1

class word
{
    public:
	word* prev;
	uint32_t contents;
	uint32_t reduces_to;
	uint8_t flags;
	word* next;
 	//The following functions are required so the structure of the doubly linked list is preserved throughout sorting.
	word(){};
	//move constructor
	word(word&& old);
	//move operator=
	word& operator=(word &&old);
};

/* The word_cmp class is used for comparing two words without using the reduces_to field
 */
struct word_cmp
{
	word_cmp();
	//The parameter strings_start should point to the value that the contents field of the word class is relative to.
	word_cmp(const char* strings_start);
	bool operator()(const word &first, const word &second);
	private:
	const char* strings_start;
};

/* The ngram_cmp class is used for comparing n-grams that are chains of words in a doubly linked list. It uses the reduces_to
 * field to compare individual words
 */
struct ngram_cmp 
{
	ngram_cmp(){};//Don't use this, it merely exists for various standard functions not to fail.
	ngram_cmp(unsigned int ngramsize,const unsigned int* word_combination_order,const optimized_combination* optimized_combo,const word* null_word,unsigned int how_many_to_ignore = 1);
	bool operator()(const word &first,const word &second);
	private:
	unsigned int n_gram_size;
	unsigned int how_many_to_ignore;
	const unsigned int *word_order;
	const optimized_combination* optimized_combo;
	const word* null_word;
};


/*
 * A function for comparing two n-grams given their starting words, their order and the null-word
 */
int ngramcmp(unsigned int n_gram_size,const word *first, const word *second,const optimized_combination* optimized_combo, const unsigned int* word_order,const word* null_word);

/*
 */
unsigned int howManyWordsAreEqual(const word* first,const  word* second,const word* null_word,unsigned int ngramsize,const unsigned int *combination,const optimized_combination *optimized_combo);



#endif
