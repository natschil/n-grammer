#include "indices_v2.h"

ngram_cmp::ngram_cmp(unsigned int ngramsize,const unsigned int* word_order,const optimized_combination *optimized_combo,const word* null_word)
{
	n_gram_size = ngramsize;
	this->word_order = word_order;
	this->optimized_combo = optimized_combo;
	this->null_word = null_word;
}

/*
 *This function compares two n-grams where the first word of the n-gram is the same.
 */
static inline int ngramcmp(unsigned int n_gram_size,const word *first, const word *second,const optimized_combination* optimized_combo, const unsigned int* word_order,const word* null_word)
{
	int min_found = 0;
	int max_found = 0;

	const word* max_found_w1 = first;
	const word* max_found_w2 = second;
	const word* min_found_w1 = first;
	const word* min_found_w2 = second;

	//MARKER5
	int cache[64];
	cache[0] = 0;

	for(size_t i = 1; i<n_gram_size;i++)
	{
		int k = word_order[i] - word_order[0];
		if(k >= 0)
		{
			if(k > max_found)
			{
				for(int l = max_found; l < k ;l++)
				{
					if(max_found_w1->next != null_word)
					{
						max_found_w1 = max_found_w1->next;
					}else
						return (max_found_w2->next == null_word) ? 0 : 1;

					if(max_found_w2->next !=  null_word)
					{
						max_found_w2 = max_found_w2->next;
					}else
						return  -1;

					cache[optimized_combo->upper[l + 1]] = max_found_w1->reduces_to - max_found_w2->reduces_to;
				}
				max_found = k;
			}
			if(cache[i] == 0)
				continue;
			else 
				return cache[i];
			
		}else
		{
			if(-k > min_found)
			{
				for(int l = min_found;l < -k ;l++)
				{
					if(min_found_w1->prev != null_word)
					{
						min_found_w1 = min_found_w1->prev;
					}else
						return (min_found_w2->prev == null_word) ? 0 : 1;

					if(min_found_w2->prev != null_word)
					{
						min_found_w2 = min_found_w2->prev;
					}else
						return  -1;

					cache[optimized_combo->lower[l]] = min_found_w1->reduces_to - min_found_w2->reduces_to;
				}
				min_found = -k ;
			}
			if(cache[i] == 0)
				continue;
			else
				return cache[i];
		}	
	}

	return 0;
}

bool ngram_cmp::operator()(const word &first,const word &second)
{
	return ngramcmp(n_gram_size,&first,&second,optimized_combo, word_order,null_word) < 0;
}

class Dict
{
	public:
	Dict(unsigned int ngramsize,const char* prefix, unsigned int buffercount,const optimized_combination *optimized_combo,const unsigned int* word_order,const word* null_word,word* buffer_bottom,uint8_t* strings_start)
	{
		this->ngramsize = ngramsize;
		this->prefix = prefix;
		this->buffercount = buffercount;
		this->null_word = null_word;
		this->buffer_bottom = buffer_bottom;
		this->strings_start = strings_start;
		this->cmp = ngram_cmp(ngramsize,word_order,optimized_combo,null_word);
		this->optimized_combo = optimized_combo;
		this->word_order = word_order;

		if(mkdir(prefix,S_IRUSR | S_IWUSR | S_IXUSR) && (errno != EEXIST))
		{
			fprintf(stderr, "Unable to create directory %s",prefix);
		}

		char buf[512];//512 Characters should be more than enough to hold 2 integers, a few characters and prefix.

		if(snprintf(buf,512,"./%s/0_%u",prefix,buffercount) >= 512)
		{
			//This should never happen.
			fprintf(stderr,"Error, poor program design");
			exit(-1);
		}
		if(mkdir(buf,S_IRUSR | S_IWUSR | S_IXUSR ) && (errno !=  EEXIST))
		{
			fprintf(stderr,"Unable to create directory %s",buf);
			exit(-1);
		}
	
		if(snprintf(buf,512,"./%s/0_%u/0.out",prefix,buffercount) >= 512) 
		{
			//This should never happen either.
			fprintf(stderr,"Error, bad coding");
			exit(-1);
		}
		outfile = fopen(buf,"a");
		if(!outfile)
		{
			fprintf(stderr,"Unable to open file %s", buf);
			exit(-1);
		}
	

	}
	void cleanUp(void)
	{
		fclose(outfile);
	};
	void writeToDisk(word* start)
	{
		word* end = (start->reduces_to + buffer_bottom) + 1;
		if(ngramsize > 1)
			std::sort(start, end, cmp);

		word* prev = start;
		long long int count = 0;
		for(word* i = start; i < end; i++)
		{
			if(i->flags & NON_STARTING_WORD)
				continue;

			if(!ngramcmp(ngramsize,i,prev,optimized_combo,word_order,null_word))
			{
				count++;
			}else
			{
				this->writeOutNGram(prev,count);
				count = 1;
				prev= i;
			}
		}

		this->writeOutNGram(prev, count);
	};
	void writeOutNGram(word* ngram_start,long long int count)
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
	}

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

Buffer::Buffer(void* internal_buffer, size_t buffer_size,size_t maximum_single_allocation,word* null_word)
{
	this->internal_buffer = internal_buffer;
	this->buffer_size = buffer_size;
	this->words_bottom_internal = buffer_size;
	this->strings_top = 0;
	this->maximum_single_allocation = maximum_single_allocation;
	this->last_word = null_word;
	//These two had better be set by hand, else garbage will come out.
	this->top_pointer = 0;
	this->bottom_pointer = 0;
};

Buffer::~Buffer()
{
	free(internal_buffer);
}

void Buffer::add_word(uint8_t* word_location, int &mmgnt_retval)
{
	word* new_word;
	words_bottom_internal -= sizeof(*new_word);
	new_word = (word*) (internal_buffer +  words_bottom_internal);

	new_word->contents =  (char*)word_location - (char*)internal_buffer;
	new_word->flags = 0;

	if((words_bottom_internal - strings_top) < maximum_single_allocation)
	{
		mmgnt_retval = -1;
	}

	last_word->next = new_word;
	new_word->prev = last_word;
	last_word = new_word;
	return;
}

void Buffer::add_null_word(word* null_word)
{
	last_word->next = null_word;
	last_word = null_word;
}

uint8_t* Buffer::allocate_for_string(size_t numbytes, int &mmgnt_retval)
{
	strings_top += numbytes;
	uint8_t* result = (uint8_t*) (internal_buffer + strings_top - numbytes);
	if((words_bottom_internal -  strings_top) < maximum_single_allocation)
	{
		mmgnt_retval = -1;
	}
	return result;
}

uint8_t* Buffer::strings_start()
{
	return (uint8_t*) internal_buffer;
}


void Buffer::rewind_string_allocation(size_t numbytes)
{
	if(strings_top >= numbytes)
		strings_top -= numbytes;
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



static unsigned long long int  factorial(unsigned int number)
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
//See the citiations in searchindexcombinations.h for justification of this
static unsigned long long int number_of_special_combinations(unsigned int number)
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

IndexCollection::IndexCollection(unsigned int buffer_size,size_t maximum_single_allocation,unsigned int ngramsize,unsigned int wordsearch_index_upto)
{
	this->buffer_size = buffer_size;
	this->maximum_single_allocation = maximum_single_allocation;
	this->ngramsize = ngramsize;
	this->numcombos = number_of_special_combinations(wordsearch_index_upto);
	this->combinations.reserve(numcombos);
	this->optimized_combinations.reserve(numcombos);
	this->prefixes.reserve(numcombos);
	this->mergeschedulers = (uint8_t (*)[MAX_K][MAX_BUFFERS]) calloc(sizeof(*this->mergeschedulers), numcombos);

	this->null_word.reduces_to =0 ;
	this->null_word.flags = 0;

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


		//MARKER5
		char* new_prefix = (char*)  malloc(6 + (3 + 1) * ngramsize + 1);//this means 999-grams are the largest we can search for.
		strcpy(new_prefix,"tmp.by");
		char* ptr = new_prefix + strlen(new_prefix);
		for(size_t i = 0; i<ngramsize; i++)
		{
			ptr += sprintf(ptr,"_%d",(int) new_word_combo[i]);
		}

		recursively_remove_directory(new_prefix);

		combinations.push_back(new_word_combo);
		optimized_combinations.push_back(optimized_combination(new_word_combo,ngramsize));
		prefixes.push_back(new_prefix);
		free(current_combo);
		combo_number++;

	 }while(++combination_itr);

	 if(combo_number != numcombos)
	 {
		fprintf(stderr,"There is an bug in index calculating\n");
		exit(-1);
	 }

	 void* first_internal_buffer = malloc(buffer_size);

	if(!first_internal_buffer)
	{
		cerr<<"Not enough memory"<<endl;
		exit(-1);
	}
	current_buffer = new Buffer(first_internal_buffer,buffer_size,maximum_single_allocation,&null_word);
	this->increment_numbuffers_in_use();
}

void IndexCollection::writeBufferToDisk(unsigned int buffercount,unsigned int rightmost_run,Buffer* buffer_to_write)
{
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

	//This step will take a long time:
	word_cmp compare((char*) strings_start);
	std::sort(buffer_bottom,buffer_top,compare);

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

	vector<Dict> current_ngrams;
	current_ngrams.reserve(numcombos);

	for(size_t i = 0; i < numcombos; i++)
	{
		current_ngrams.push_back(
				Dict(ngramsize,prefixes[i],buffercount,&optimized_combinations[i],combinations[i],&null_word,buffer_bottom,strings_start)
				);
	}


	for(word* current_word = buffer_bottom; current_word != buffer_top;)
	{
		for(size_t i = 0; i< numcombos; i++)
		{
			current_ngrams[i].writeToDisk(current_word);
		}
		current_word = (buffer_bottom + current_word->reduces_to) + 1;
	}

	this->decrement_numbuffers_in_use();
	delete buffer_to_write;
	//At this point do all merging
	for(unsigned int i = 0; i< numcombos; i++)
	{
		current_ngrams[i].cleanUp();
		schedule_next_merge(0,buffercount,rightmost_run,mergeschedulers+i,prefixes[i]);
	}
	//We've written out the old buffer, and merged up as far as it was possible.
	//That's all this functions does.
	return;
}

void IndexCollection::makeNewBuffer(void)
{
	void* new_internal_buffer = malloc(buffer_size);
	if(!new_internal_buffer)
	{
		cerr<<"Not enough memory, exiting"<<endl;
		exit(-1);
	}
	current_buffer = new Buffer(new_internal_buffer, buffer_size,maximum_single_allocation,&null_word);
	this->increment_numbuffers_in_use();
	return;

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

void IndexCollection::writeMetadata(FILE* metadata_file)
{
	fprintf(metadata_file,"Indexes:\n");
	for(size_t i = 0; i< numcombos;i++)
	{
		fprintf(metadata_file,"\t%s\n",prefixes[i] + 4);//To remove the preceding "tmp.".
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
		char* original_path = (char*) malloc(strlen(prefixes[i]) + 256 );//More than enough
		sprintf(original_path, "%s/%d_0",prefixes[i],k);
		rename(original_path,finalpath);
		recursively_remove_directory(prefixes[i]);
		free(finalpath);
		free(original_path);
	}
	return;
}

