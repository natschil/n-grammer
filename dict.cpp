#include "dict.h"
DictCollection::DictCollection(
				unsigned int ngramsize,
				unsigned int buffercount,
				vector<unsigned int*> &combinations,
				vector<optimized_combination> &optimized_combinations,
				vector<char*> &prefixes,
				const word* null_word,
				word* buffer_bottom,
				uint8_t* strings_start
				)
				: combinations(combinations) , optimized_combinations(optimized_combinations)
{
	this->ngramsize = ngramsize;
	this->buffercount = buffercount;
	this->numcombos = combinations.size();

	for(unsigned int i = 0; i< combinations.size(); i++)
	{
		dictionaries.push_back(Dict(
					ngramsize,
				       	prefixes[i],
					buffercount,
					&optimized_combinations[i],
				       	combinations[i],
					null_word,
					buffer_bottom,
					strings_start
					));
	}

}

static unsigned int howManyWordsAreEqualInCombination(const unsigned int* first_combo, const unsigned int *second_combo,unsigned int ngramsize)
{
	unsigned int i;
	for(i = 1; i <ngramsize; i++)
	{	
		if((first_combo[i] - first_combo[0]) != (second_combo[i] - second_combo[0]))
			break;
	}
	return i;
}

void DictCollection::writeRangeToDisk(word* begin, word* end,const word* null_word)
{
	//beginnings_stack[i] holds a pointer to where reading should start if i words are equal to the previous combination
	
	vector<word*> beginnings_stack;
	beginnings_stack.resize(ngramsize + 1);

	for(size_t i = 0; i < ngramsize + 1; i++)
	{
		beginnings_stack[i] = begin;
	}

	size_t current_combo = 0;
	long long wordcount = 0;
	int equal_words_to_next_combination;

	equal_words_to_next_combination = (current_combo + 2 > numcombos)?
	       	0: howManyWordsAreEqualInCombination(combinations[current_combo],combinations[current_combo+1],ngramsize);

	//Iterate over all words.
	for(word* current_word_ptr = begin; ; current_word_ptr++)
	{
		int equal_words; //The number of words that are equal under this ordering to the last n-gram encountered.
		if(current_word_ptr == end)
			equal_words = 0;
		else
			equal_words = howManyWordsAreEqual(
					beginnings_stack[ngramsize], 
					current_word_ptr,
					null_word,
					ngramsize,
				       	combinations[current_combo],
					&optimized_combinations[current_combo]
					);

		//If not all words were equal.
		if(equal_words < (int)ngramsize)
		{
			//wordcount could be 0 if all words in the range have the NON_STARTING_WORD flag set.
			if(wordcount)
				dictionaries[current_combo].writeOutNGram(beginnings_stack[ngramsize], wordcount);
			wordcount = 0;

			//If fewever words were equal than the number of equal words that the next combination has
			//This means that we can write out the next combination now.
			if((int)equal_words < equal_words_to_next_combination)
			{
				//Increase the combination, and sort it.
				current_combo++;
				std::sort(
						beginnings_stack[equal_words_to_next_combination],
						current_word_ptr,
						ngram_cmp(ngramsize,combinations[current_combo],&optimized_combinations[current_combo],null_word,equal_words_to_next_combination)
						);

				current_word_ptr = beginnings_stack[equal_words_to_next_combination];
				for(size_t i = equal_words_to_next_combination; i < ngramsize+1; i++)
				{
					beginnings_stack[i] = current_word_ptr;
				}

				equal_words_to_next_combination = (current_combo + 2 > numcombos )?
	       			0: howManyWordsAreEqualInCombination(combinations[current_combo],combinations[current_combo+1],ngramsize);

			}else 
			{
				//We've reached the end of a group of n-grams where the first few words are the same
				//We update where we will start the subsequent time we backtrack.
				for(size_t i = equal_words; i < ngramsize  ; i++)
				{
					beginnings_stack[i + 1] = current_word_ptr;
				}

				//We go back to the last combination that has the current number of equal words
				// words equal to this one.
				unsigned int new_combo = current_combo;
				while(new_combo)
				{
					int to_prev = howManyWordsAreEqualInCombination(combinations[current_combo],combinations[new_combo - 1],ngramsize);
					if(equal_words < to_prev)
					{
						new_combo--;
					}else
					{
						break;
					}
				}

				current_combo = new_combo;
				equal_words_to_next_combination = (current_combo + 2 > numcombos )?
	       				0: howManyWordsAreEqualInCombination(combinations[current_combo],combinations[current_combo+1],ngramsize);
			}

			if(current_word_ptr == end)
				break;
		}

		if(!(current_word_ptr->flags & NON_STARTING_WORD))
		{
			wordcount++;
		}

	}
}

void DictCollection::cleanUp(void)
{
	for(unsigned int i = 0; i< numcombos;i++)
	{
		dictionaries[i].cleanUp();
	}
}



Dict::Dict(
		unsigned int ngramsize,
		const char* prefix,
	       	unsigned int buffercount,
		const optimized_combination *optimized_combo,
		const unsigned int* word_order,
		const word* null_word,
		word* buffer_bottom,
		uint8_t* strings_start
		)
{
	this->ngramsize = ngramsize;
	this->prefix = prefix;
	this->buffercount = buffercount;
	this->optimized_combo = optimized_combo;
	this->word_order = word_order;
	this->null_word = null_word;
	this->buffer_bottom = buffer_bottom;
	this->strings_start = strings_start;


	if(mkdir(prefix,S_IRUSR | S_IWUSR | S_IXUSR) && (errno != EEXIST))
	{
		fprintf(stderr, "Unable to create directory %s",prefix);
	}

	stringstream buf("");
	buf<<"./"<<prefix<<"/0_"<<buffercount;

	if(mkdir(buf.str().c_str(),S_IRUSR | S_IWUSR | S_IXUSR ) && (errno !=  EEXIST))
	{
		fprintf(stderr,"Unable to create directory %s",buf.str().c_str());
		exit(-1);
	}
	buf<<"/0.out";

	outfile = fopen(buf.str().c_str(),"a");
	if(!outfile)
	{
		fprintf(stderr,"Unable to open file %s", buf.str().c_str());
		exit(-1);
	}
}
void Dict::cleanUp(void)
{
	fclose(outfile);
	return;
};





void Dict::writeOutNGram(word* ngram_start,long long int count)
{
	//MARKER5
	const uint8_t *words_in_ngram[64];
	word* tmp = ngram_start;
	int skip = 0;
	for(size_t j = 0; j < optimized_combo->lower_size; j++)
	{
		if(tmp->prev == null_word)
		{
			skip = 1;
			break;
		}else
			tmp = tmp->prev;
		words_in_ngram[optimized_combo->lower[j]] = strings_start + tmp->contents;
	}

	tmp = ngram_start;
	words_in_ngram[0] = strings_start + tmp->contents;
	for(size_t j = 1; j < optimized_combo->upper_size;j++)
	{
		if(tmp->next == null_word)
		{
			skip = 1;
			break;
		}else
			tmp = tmp->next;
		words_in_ngram[optimized_combo->upper[j]] = strings_start + tmp->contents;
	}
	for(size_t j = 0;!skip && ( j <ngramsize); j++)
	{
		ulc_fprintf(outfile,"%U",words_in_ngram[j]);
		if(j != (ngramsize - 1))
			fprintf(outfile," ");
	}
	if(!skip)
		fprintf(outfile,"\t%lld\n",count);
	return;
}
