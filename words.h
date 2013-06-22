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
	ngram_cmp(unsigned int ngramsize,const unsigned int* word_combination_order,const optimized_combination* optimized_combo,const word* null_word,unsigned int how_many_to_ignore);
	bool operator()(const word &first,const word &second);
	private:
	unsigned int n_gram_size;
	unsigned int how_many_to_ignore;
	const unsigned int *word_order;
	const optimized_combination* optimized_combo;
	const word* null_word;
	const char* strings_start;
};


unsigned int howManyWordsAreEqual(const word* first,const  word* second,const word* null_word,unsigned int ngramsize,const unsigned int *combination,const optimized_combination *optimized_combo);


/*
 * A function for comparing two n-grams given their starting words, their order and the null-word
 * If reduces_to_is_set is true, it uses the reduces_to field for word comparisons, otherwise it uses strcmp.
 * Note that strings_start should only be set if reduces_to_is_set is false, and that it's value is ignored if it is true.
 */
template <bool reduces_to_is_set>
int ngramcmp(
		unsigned int n_gram_size,
		const word *first,
	       	const word *second,
		const optimized_combination* optimized_combo, 
		const unsigned int* word_order,
		const word* null_word,
		unsigned int how_many_to_ignore,
		const char* strings_start
		)
{
	int min_found = 0;
	int max_found = 0;

	const word* max_found_w1 = first;
	const word* max_found_w2 = second;
	const word* min_found_w1 = first;
	const word* min_found_w2 = second;

	//MARKER5
	int cache[64] = {0};
	if(!how_many_to_ignore)
	{
		if(reduces_to_is_set)
			cache[0] = first->reduces_to - second->reduces_to;
		else
			cache[0] = strcmp(strings_start + first->contents,strings_start + second->contents);
	}

	for(size_t i = how_many_to_ignore; i<n_gram_size;i++)
	{
		int relative_position = word_order[i] - word_order[0];
		if(relative_position >= 0)
		{
			if(relative_position > max_found)
			{
				for(int j = max_found; j < relative_position;j++)
				{
					if(max_found_w1->next != null_word)
					{
						max_found_w1 = max_found_w1->next;
						if(max_found_w2->next != null_word)
						{
							max_found_w2 = max_found_w2->next;
							if(reduces_to_is_set)
								cache[optimized_combo->upper[j+1]] = max_found_w1->reduces_to - max_found_w2->reduces_to;
							else
								cache[optimized_combo->upper[j+1]] = strcmp(strings_start +max_found_w1->contents,strings_start +max_found_w2->contents);

						}else
						{
							cache[optimized_combo->upper[j+1]] = -1;
						}

					}else if(max_found_w2->next != null_word)
					{
						max_found_w2 = max_found_w2->next;
						cache[optimized_combo->upper[j+1]] = 1;
					}else
					{
						cache[optimized_combo->upper[j+1]] = 0;
					}
				}
				max_found = relative_position;
			}
			if(cache[i] == 0)
				continue;
			else 
				return cache[i];
			
		}else
		{
			if(-relative_position > min_found)
			{
				for(int j = min_found;j < -relative_position ;j++)
				{
					if(min_found_w1->prev != null_word)
					{
						min_found_w1 = min_found_w1->prev;
						if(min_found_w2->prev != null_word)
						{
							min_found_w2 = min_found_w2->prev;
							if(reduces_to_is_set)
								cache[optimized_combo->lower[j]] = min_found_w1->reduces_to - min_found_w2->reduces_to;
							else
								cache[optimized_combo->lower[j]] = strcmp(strings_start +min_found_w1->contents,strings_start +min_found_w2->contents);
						}else
						{
							cache[optimized_combo->lower[j]] = -1;
						}
					}else if(min_found_w2->prev != null_word)
					{
						min_found_w2 = min_found_w2->next;
						cache[optimized_combo->lower[j]] = 1;
					}else
					{
						cache[optimized_combo->lower[j]] = 0;
					}
				}
				min_found = -relative_position ;
			}
			if(cache[i] == 0)
				continue;
			else
				return cache[i];
		}	
	}

	return 0;
}



#endif
