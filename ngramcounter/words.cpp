#include "words.h"

word::word(word&& old)
{
	prev = old.prev;
	next = old.next;
	contents = old.contents;
	reduces_to = old.reduces_to;
	flags = old.flags;
	prev->next = this;
	next->prev = this;	
	this->next = old.next;
	this->prev = old.prev;
};

word& word::operator=(word&& old)
{			
	prev = old.prev;
	next = old.next;
	flags = old.flags;
	contents = old.contents;
	reduces_to = old.reduces_to;

	prev->next = this;
	next->prev = this;	

	this->next = old.next;
	this->prev = old.prev;
	return *this;
};

word_cmp::word_cmp(const char* strings_start)
{
	this->strings_start = strings_start;
}
bool word_cmp::operator()(const word &first, const word &second)
{
	return strcmp(strings_start + first.contents, strings_start + second.contents) < 0;
}

ngram_cmp::ngram_cmp(unsigned int ngramsize,const unsigned int* word_order,const optimized_combination *optimized_combo,const word* null_word,unsigned int how_many_to_ignore)
{
	n_gram_size = ngramsize;
	this->word_order = word_order;
	this->optimized_combo = optimized_combo;
	this->null_word = null_word;
	this->how_many_to_ignore = how_many_to_ignore;
}


unsigned int howManyWordsAreEqual(const word* first,const  word* second,const word* null_word,unsigned int n_gram_size,const unsigned int *word_order,const optimized_combination *optimized_combo)
{
	int min_found = 0;
	int max_found = 0;

	const word* max_found_w1 = first;
	const word* max_found_w2 = second;
	const word* min_found_w1 = first;
	const word* min_found_w2 = second;

	//MARKER5
	int cache[64];
	cache[0] = first->reduces_to - second->reduces_to;

	for(size_t i = 0; i<n_gram_size;i++)
	{
		int relative_position = word_order[i] - word_order[0];
		if(relative_position >= 0)
		{
			if(relative_position > max_found)
			{
				for(int j = max_found; j < relative_position;j++)
				{
					if(max_found_w1 != null_word)
					{
						max_found_w1 = max_found_w1->next;
					}
					if(max_found_w2 != null_word)
					{
						max_found_w2 = max_found_w2->next;
					}
					cache[optimized_combo->upper[j+1]] = max_found_w1->reduces_to - max_found_w2->reduces_to;
				}
				max_found = relative_position;
			}
			if(cache[i] == 0)
				continue;
			else 
				return i;
			
		}else
		{
			if(-relative_position > min_found)
			{
				for(int j = min_found;j < -relative_position;j++)
				{
					if(min_found_w1 != null_word)
					{
						min_found_w1 = min_found_w1->prev;
					}
					if(min_found_w2 != null_word)
					{
						min_found_w2 = min_found_w2->prev;
					}
					cache[optimized_combo->lower[j]] = min_found_w1->reduces_to - min_found_w2->reduces_to;
				}
				min_found = -relative_position ;
			}
			if(cache[i] == 0)
				continue;
			else
				return i;
		}	
	}

	return n_gram_size;
}

bool ngram_cmp::operator()(const word &first,const word &second)
{
	return ngramcmp<true>(n_gram_size,&first,&second,optimized_combo, word_order,null_word,how_many_to_ignore,NULL) < 0;
}
