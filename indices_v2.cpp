#include "indices_v2.h"

ngram_cmp::ngram_cmp(unsigned int ngramsize,const unsigned int* word_order,const word* null_word)
{
	n_gram_size = ngramsize;
	this->word_order = word_order;
	this->null_word = null_word;
}

bool ngram_cmp::operator()(const word &first,const word &second)
{
	//TODO: heavily optimize this:
	bool prevres = 0;
	for(size_t i = 0; i<n_gram_size;i++)
	{
		int k = word_order[i] - word_order[0];
		word *cur = &first;
		word *cur2 = &second;
		if(k >= 0)
		{
			for(;k;k--)
			{
				if(cur->next != null_word)
				{
					cur = cur->next;
				}else//Put null words at the end:
				{
					return true;
				}
				if(cur2->next !=  null_word)
				{
					cur2 = cur2->next;
				}else
					return false;
			
			}
		}else
		{
			for(;k;k++)
			{
				if(cur->next != null_word)
				{
					cur = cur->next;
				}else
				{
					return true;
				}
				if(cur2->next != null_word)
				{
					cur2 = cur2->next;
				}else
					return false;
			}
		}
		if(!(prevres = (cur->reduces_to - cur2->reduces_to)))
		{
			continue;
		}else
			return prevres < 0;
	}

	return false;
}

class Dict
{
	public:
	Dict(ngram_cmp &cmp,unsigned int ngramsize,const char* prefix, unsigned int buffercount)
	{
		this->ngramsize = ngramsize;
		this->prefix = prefix;
		this->buffercount = buffercount;
		this->cmp = cmp;

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
	void writeToDisk(word* start)
	{
		word* end = start->reduces_to + 1;
		std::sort(start, end, cmp);
		//TODO: finish this, so that it basically writes out all of the strings.
		//TODO: also finish everything else that is relevant
	}

	private:
		ngram_cmp cmp;
		unsigned int ngramsize;
		unsigned int buffercount;
		const char* prefix;
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

	new_word->contents = word_location;

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
	word* words_bottom = buffer_to_write->words_bottom();
	word* words_top = buffer_to_write->words_top();

	//This step will take a long time:
	std::sort(words_bottom,words_top);

	//We now see which words are equal: TODO: move this into the sorting step.
	word* prev_ptr = words_top -1;
	for(word* ptr = words_top-1; ptr >= words_bottom; ptr--)
	{
		if(strcmp((const char*) ptr->contents,(const char*) prev_ptr->contents))
		{
			prev_ptr = ptr;
		}
		ptr->reduces_to = prev_ptr;
	}

	vector<Dict> current_ngrams;
	current_ngrams.reserve(numcombos);

	for(size_t i = 0; i < numcombos; i++)
	{
		ngram_cmp cur(ngramsize,combinations[i]);
		current_ngrams.push_back(Dict(cur,ngramsize,prefixes[i],buffercount));
	}


	NGram new_ngram;
	for(word* current_word = words_bottom; current_word != words_top;)
	{
		const word *previous_word_reduces_to = current_word->reduces_to;
		while((current_word != words_top) && (!(previous_word_reduces_to - current_word->reduces_to)))
		{
			for(size_t i = 0; i < numcombos; i++)
			{
				new_ngram.ngram.clear();
				new_ngram.ngram.resize(ngramsize); 
				int skip = 0;
				word* temporary_pointer = current_word;
				for(size_t k = 0; k < optimized_combinations[i].lower_size; k++)
				{

					temporary_pointer = temporary_pointer->prev;
					if(temporary_pointer == &null_word)
					{
						skip = 1;
						break;
					}
					new_ngram.ngram[optimized_combinations[i].lower[k]] = temporary_pointer;
				}
				temporary_pointer = current_word;
				for(size_t k = 0;!skip &&( k < optimized_combinations[i].upper_size);k++)
				{
					if(temporary_pointer == &null_word)
					{
						skip = 1;
						break;
					}

					new_ngram.ngram[optimized_combinations[i].upper[k]] = temporary_pointer;
					temporary_pointer = temporary_pointer->next;

				}

				if(!skip)
				{
					current_ngrams[i].addNGram(new_ngram);
				}
				else
					skip = 0;
			}
			current_word++;
		}
		//We now write out any n-grams that we have:
		for(size_t i = 0; i< numcombos; i++)
		{
			current_ngrams[i].writeToDisk();
			current_ngrams[i].clear();
		}	
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

