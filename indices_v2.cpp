#include "indices_v2.h"


Buffer::Buffer(void* internal_buffer, size_t buffer_size,size_t maximum_single_allocation,word* null_word)
{
	this->internal_buffer = internal_buffer;
	this->is_full = false;
	this->buffer_size = buffer_size;
	this->maximum_single_allocation = maximum_single_allocation;

	//Set the pointers so that they indicate the buffer is empty.
	this->words_bottom_internal = buffer_size;
	this->strings_top = 0;

	this->last_word = null_word;
	this->null_word = null_word;

	//These two had better be set by hand later on at some point.
	this->top_pointer = 0;
	this->bottom_pointer = 0;
};


void Buffer::add_word(uint8_t* word_location)
{
	word* new_word;
	words_bottom_internal -= sizeof(*new_word);
	new_word = (word*) (internal_buffer +  words_bottom_internal);

	new_word->contents =  (const char*)word_location - (const char*)internal_buffer;
	new_word->flags = 0;

	if((words_bottom_internal - strings_top) < maximum_single_allocation)
	{
		is_full = true;
	}

	last_word->next = new_word;
	new_word->prev = last_word;
	last_word = new_word;
	return;
}

void Buffer::add_null_word()
{
	last_word->next = null_word;
	last_word = null_word;
}

void Buffer::advance_to(size_t nmeb)
{
	words_bottom_internal = buffer_size -  nmeb * sizeof(word);
}

void Buffer::set_current_word_as_last_word()
{
	last_word = (word*) (internal_buffer + words_bottom_internal);
}

void Buffer::add_word_at_start(const uint8_t* string)
{
	if(words_bottom_internal == buffer_size)
	{
		fprintf(stderr, "Too little space in buffer reserved\n");
		exit(-1);
	}
	word* new_word = (word*) (internal_buffer + words_bottom_internal);
	words_bottom_internal += sizeof(word);

	new_word->contents = (const char*) string - (const char*)internal_buffer;
	new_word->flags = 0;

	last_word->prev = new_word;
	new_word->next =  last_word;

	last_word = new_word;

}
void Buffer::add_null_word_at_start()
{
	last_word->prev = null_word;
	last_word = null_word;
}

uint8_t* Buffer::allocate_for_string(size_t numbytes)
{
	strings_top += numbytes;
	uint8_t* result = (uint8_t*) (internal_buffer + strings_top - numbytes);
	if((words_bottom_internal -  strings_top) < maximum_single_allocation)
	{
		is_full = true;
	}
	return result;
}

void Buffer::rewind_string_allocation(size_t numbytes)
{
	if(strings_top >= numbytes)
		strings_top -= numbytes;
}

uint8_t* Buffer::strings_start()
{
	return (uint8_t*) internal_buffer;
}

void Buffer::set_top_pointer(size_t nmeb)
{
	top_pointer = buffer_size - nmeb* sizeof(word);
}

void Buffer::set_bottom_pointer(size_t nmeb)
{
	bottom_pointer = words_bottom_internal + nmeb*sizeof(word);
}

word* Buffer::words_top()
{
	return (word*)(internal_buffer + top_pointer); 
}

word* Buffer::words_bottom()
{
	return (word*)(internal_buffer + bottom_pointer);
}

word* Buffer::words_buffer_top()
{
	return (word*) (internal_buffer + buffer_size);
}

word* Buffer::words_buffer_bottom()
{
	return (word*) (internal_buffer + words_bottom_internal);
}

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
				) ;
		void writeToDisk(word* start);
		void writeRangeToDisk(word* begin, word* end,const word* null_word);
		void cleanUp(void);
	private:
		unsigned int ngramsize;
		unsigned int buffercount;
		unsigned int numcombos;
		vector<Dict> dictionaries;

		vector<unsigned int*> &combinations;
		vector<optimized_combination> &optimized_combinations;
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
	void writeOutNGram(word* ngram_start,long long int count);
    private:
	const optimized_combination *optimized_combo;
	const unsigned int* word_order;

	unsigned int ngramsize;
	unsigned int buffercount;
	const char* prefix;
	const word* null_word;
	word* buffer_bottom;
	uint8_t* strings_start;
	FILE* outfile;
};

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
	
	word* beginnings_stack[ngramsize + 1];

	for(size_t i = 0; i < ngramsize + 1; i++)
	{
		beginnings_stack[i] = begin;
	}
	unsigned int shared_equal_words_matrix[numcombos*numcombos];
	for(size_t i = 0; i < numcombos; i++)
	{
		for(size_t j = 0; j < numcombos; j++)
		{
			shared_equal_words_matrix[numcombos*i + j] = howManyWordsAreEqualInCombination(combinations[i],combinations[j],ngramsize);
		}
	}

	size_t current_combo = 0;
	long long wordcount = 0;
	unsigned int equal_words_to_next_combination;

	equal_words_to_next_combination = (current_combo + 2 > numcombos)?
	       	0: shared_equal_words_matrix[numcombos*current_combo + (current_combo + 1)];

	//Iterate over all words.
	for(word* current_word_ptr = begin; ; current_word_ptr++)
	{
		unsigned int equal_words; //The number of words that are equal under this ordering to the last n-gram encountered.
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
		if(equal_words < ngramsize)
		{
			//wordcount could be 0 if all words in the range have the NON_STARTING_WORD flag set.
			if(wordcount)
				dictionaries[current_combo].writeOutNGram(beginnings_stack[ngramsize], wordcount);
			wordcount = 0;

			//If fewever words were equal than the number of equal words that the next combination has
			//This means that we can write out the next combination now.
			if(equal_words < equal_words_to_next_combination)
			{
				//Increase the combination, and sort it.
				current_combo++;
				std::sort(
						beginnings_stack[equal_words_to_next_combination],
						current_word_ptr,
						ngram_cmp(ngramsize,combinations[current_combo],&optimized_combinations[current_combo],null_word,equal_words_to_next_combination)
						);

				current_word_ptr = beginnings_stack[equal_words_to_next_combination];
				for(size_t i = equal_words_to_next_combination + 1; i < ngramsize+1; i++)
				{
					beginnings_stack[i] = current_word_ptr;
				}


				equal_words_to_next_combination = (current_combo + 2 > numcombos )?
	       				0: shared_equal_words_matrix[numcombos*current_combo + (current_combo + 1)];

			}else if(equal_words < ngramsize )
			{

				if(current_word_ptr == end)
					break;
				
				//We've reached the end of a group of n-grams where the first few words are the same
				//We update where we will start the subsequent time we backtrack.
				for(size_t i = equal_words; i < ngramsize  ; i++)
				{
					beginnings_stack[i + 1] = current_word_ptr;
				}

				//We go back to the last combination that has the current number of equal words
				// words equal to or larger than the current one
				unsigned int new_combo = current_combo;
				while(new_combo)
				{
					unsigned int to_prev = shared_equal_words_matrix[numcombos*current_combo + (new_combo - 1)];
					if(equal_words < to_prev)
					{
						new_combo--;
					}else
					{
						break;
					}
				}
				if(new_combo < current_combo)
				{
					current_combo = new_combo;
	
					equal_words_to_next_combination = (current_combo + 2 > numcombos )?
		       				0: shared_equal_words_matrix[numcombos*current_combo + (current_combo + 1)];
				}
			}
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



static unsigned long long int factorial(unsigned int number)
{
	if(!number) return 1;
	int result = number;
	while(--number)
	{
		result *= number;
	}
	return result;
}

//Computes n C [n/2] where "n C r"  is  n choose r, and [n/2] means floor(n/2)
//See the citiations in searchindexcombinations.h for justification for this representing the number of indexes
//needed for multi-word retrieval for n-grams.
//As this function is only called rarely, it isn't overly optimized.
unsigned long long int number_of_special_combinations(unsigned int number)
{
	unsigned long long int result = factorial(number)/
					( 
					 	factorial((number % 2)?(number - 1)/2:number/2)
					 *
					 	factorial(
							number - 
							((number % 2) ? (number -1)/2:number/2)
							)
					);
	return result;	
}

IndexCollection::IndexCollection(unsigned int ngramsize,unsigned int wordsearch_index_upto,bool is_pos_supplement_index)
{
	this->ngramsize = ngramsize;
	this->numcombos = number_of_special_combinations(wordsearch_index_upto);
	this->combinations.reserve(numcombos);
	this->optimized_combinations.reserve(numcombos);
	this->prefixes.reserve(numcombos);
	this->mergeschedulers = (uint8_t (*)[2*MAX_BUFFERS-1]) calloc(sizeof(*this->mergeschedulers), numcombos);

	this->buffercounter = 0;
	this->num_being_read = 0;
	this->is_pos_supplement_index = is_pos_supplement_index;

	if(!this->mergeschedulers)
	{
		cerr<<"Not enough memory, exiting"<<endl;
		exit(-1);
	}

	if(wordsearch_index_upto != 1)
	{
		 fprintf(stderr, "Generating %d indices\n",(int) numcombos);
	}


	CombinationIterator combination_itr(wordsearch_index_upto);
	size_t combo_number = 0;
	do
	{
 		unsigned int* current_combo = *combination_itr;		 
		unsigned int* new_word_combo = (unsigned int*) malloc(ngramsize * sizeof(new_word_combo));


	 	size_t j = 0;
		for(size_t i = 0; i < wordsearch_index_upto;i++)
		{
			if(current_combo[i] != (wordsearch_index_upto - 1))
			{
				new_word_combo[j] = current_combo[i];
				j++;
			}else
			{
				for(size_t k = wordsearch_index_upto - 1; k < ngramsize; k++)
				{
					new_word_combo[j] = (int) k;		
					j++;
				}
			}
		}

		combinations.push_back(new_word_combo);
		free(current_combo);
		combo_number++;
	}while(++combination_itr);

	std::sort(combinations.begin(),combinations.end(), 
			[&ngramsize](unsigned int* first, unsigned int* second){
			for(unsigned int i= 0; i< ngramsize; i++)
			{
				if((first[i] - first[0]) != (second[i] - second[0]))
				{
					return (first[i] - first[0]) < (second[i] - second[0]);
				}
			}
			return false;

			});

	for(unsigned int i = 0; i<combo_number; i++)
	{
		optimized_combinations.push_back(optimized_combination(combinations[i],ngramsize));

		stringstream new_prefix_stream("");
		if(!is_pos_supplement_index)
		{
			new_prefix_stream<<"tmp.by";
		}else
		{
			new_prefix_stream<<"tmp.pos_supplement_index";
		}
		for(size_t j = 0; j<ngramsize; j++)
		{
			new_prefix_stream<<"_";
			if(is_pos_supplement_index)
			{
				if((combinations[i])[j] == 0)
					new_prefix_stream<<'i';
				else if((combinations[i])[j] == 1)
					new_prefix_stream<<'c';
				else if((combinations[i])[j] == 2)
					new_prefix_stream<<'l';
			}else
			{
				new_prefix_stream<<(int) (combinations[i])[j];
			}
		}

		recursively_remove_directory(new_prefix_stream.str().c_str());
		prefixes.push_back(strdup(new_prefix_stream.str().c_str()));
	}

	if(combo_number != numcombos)
	{
		fprintf(stderr,"There is an bug in index calculating\n");
		exit(-1);
	}
}

IndexCollection::~IndexCollection()
{
	for(size_t i = 0; i< numcombos;i++)
	{
		free(combinations[i]);
		free(prefixes[i]);
	}
	free(mergeschedulers);
}

void IndexCollection::writeBufferToDisk(unsigned int buffercount,unsigned int rightmost_run,Buffer* buffer_to_write,word* null_word)
{
	fprintf(stdout,"(");
	fflush(stdout);
	word* buffer_bottom = buffer_to_write->words_buffer_bottom();
	word* buffer_top = buffer_to_write->words_buffer_top();
	uint8_t* strings_start = buffer_to_write->strings_start();

	for(word* ptr = buffer_bottom; ptr < buffer_to_write->words_bottom(); ptr++)
	{
		ptr->flags |= NON_STARTING_WORD;
	}

	for(word* ptr = buffer_to_write->words_top(); ptr < buffer_to_write->words_buffer_top(); ptr++)
	{
		ptr->flags |= NON_STARTING_WORD;
	}

	//This step will take a long time: TODO: Think about whether sorting by whole n-grams by first combo makes sense here.
	word_cmp compare((char*) strings_start);
	std::sort(buffer_bottom,buffer_top,
			[&](const word &first, const word &second) -> bool
			{
				const word* first_cur = &first;
				const word* second_cur = &second;
				for(unsigned int counter = 0;;)
				{
					int res = strcmp((const char*)strings_start + first_cur->contents, (const char*)strings_start + second_cur->contents);
					if(res > 0)
						return false;
					else if(res)
						return true;

					if(++counter < ngramsize)
					{
						if(first_cur->next == null_word)
							return (second_cur->next == null_word)?false:false;
						else if(second_cur->next == null_word)
							return true;
	
						first_cur = first_cur->next;
						second_cur = second_cur->next;
					}else
						return res > 0;
				}
				return false;

			}
			);

	//We now see which words are equal: TODO: move this into the sorting step.
	word* prev_ptr = buffer_top-1;

	for(word* ptr = buffer_top-1; ptr >= buffer_bottom; ptr--)
	{
		if(strcmp((const char*)strings_start + ptr->contents,(const char*) strings_start + prev_ptr->contents))
		{
			prev_ptr = ptr;
		}
		ptr->reduces_to = prev_ptr-buffer_bottom;
	}

	DictCollection current_ngrams(
			ngramsize,
			buffercount,
			combinations,
			optimized_combinations,
			prefixes,
			null_word,
			buffer_bottom,
			strings_start
			);

	for(word* current_word = buffer_bottom; current_word != buffer_top;)
	{
		current_ngrams.writeRangeToDisk(current_word,buffer_bottom + current_word->reduces_to + 1, null_word);
		current_word = (buffer_bottom + current_word->reduces_to) + 1;
	}

	fprintf(stdout,")");
	fflush(stdout);
	current_ngrams.cleanUp();

	//At this point do all merging
	for(unsigned int i = 0; i< numcombos; i++)
	{
		schedule_next_merge(0,buffercount,rightmost_run,mergeschedulers+i,prefixes[i]);
	}
	//We've written out the old buffer, and merged up as far as it was possible.
	//That's all this functions does.
	return;
}

void IndexCollection::writeMetadata(FILE* metadata_file)
{
	if(!is_pos_supplement_index)
	{
		fprintf(metadata_file,"Indexes:\n");
		for(size_t i = 0; i< numcombos;i++)
		{
			fprintf(metadata_file,"\t%s\n",prefixes[i] + 4);//To remove the preceding "tmp.".
		}
	}else
	{
		fprintf(metadata_file, "POSIndexes:\tyes\n");
	}
	return;
}

void IndexCollection::copyToFinalPlace(int k)
{
	for(size_t i = 0; i < numcombos;i++)
	{
		//TODO: Copy any relevant metadata here...
		char* finalpath = (char*) malloc(strlen(prefixes[i]) * sizeof(*finalpath) + 1);
		strcpy(finalpath,prefixes[i] + 4);//To remove preceding "tmp."
		recursively_remove_directory(finalpath);
		char* original_path = (char*) malloc(strlen(prefixes[i]) + 1 + 4 + 1 + 1 );
		sprintf(original_path, "%s/%d_0",prefixes[i],k);
		rename(original_path,finalpath);
		recursively_remove_directory(prefixes[i]);
		free(finalpath);
		free(original_path);
	}
	return;
}

void IndexCollection::add_range_to_schedule(off_t start, off_t end)
{
	schedule_entry new_entry;
	new_entry.start = start;
	new_entry.end = end;

	#pragma omp critical(buffer_scheduling)
	{
		#pragma omp flush
		unread_ranges.push(new_entry);
		#pragma omp flush
	}
}
bool IndexCollection::mark_the_fact_that_a_range_has_been_read_in()
{
	bool was_last = false;
	#pragma omp critical(buffer_scheduling)
	{
		#pragma omp flush
		num_being_read--;
		if(!num_being_read && unread_ranges.empty())
		{
			was_last =true; 
			
		}
		#pragma omp flush
	}
	return was_last;
}
void IndexCollection::writeEmptyLastBuffer(int buffercount)
{
	void* tmp_internal_buffer = malloc((MAX_WORD_SIZE + 1) + sizeof(word));
	word null_word;
	Buffer* empty = new Buffer(tmp_internal_buffer,MAX_WORD_SIZE + 1 + sizeof(word),(MAX_WORD_SIZE + 1) + sizeof(word),&null_word);
	empty->set_top_pointer(0);
	empty->set_bottom_pointer(0);
	this->writeBufferToDisk(buffercount,1,empty,&null_word);
	delete empty;
	free(tmp_internal_buffer);
}
pair<schedule_entry,int> IndexCollection::get_next_schedule_entry()
{
	pair<schedule_entry,int> result;

	#pragma omp critical(buffer_scheduling)
	{
		#pragma omp flush
		if(!unread_ranges.empty())
		{
			schedule_entry top = unread_ranges.top();
			unread_ranges.pop();
			result.first = top;
			result.first.buffercount = buffercounter++;
			result.second = 1;
			num_being_read++;
		}else
		{
			result.second = 0;
		}
		#pragma omp flush
	}
	return result;
}

