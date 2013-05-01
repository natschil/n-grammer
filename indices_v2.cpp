#include "indices_v2.h"

ngram_cmp::ngram_cmp(unsigned int ngramsize,const unsigned int *combination)
{
	this->word_order = combination;
	n_gram_size = ngramsize;
}
ngram_cmp::ngram_cmp(const ngram_cmp &prev)
{
	this->word_order = prev.word_order;
	this->n_gram_size = prev.n_gram_size;
}
ngram_cmp ngram_cmp::operator=(const ngram_cmp &prev)
{
	this->word_order = prev.word_order;
	this->n_gram_size = prev.n_gram_size;
	return *this;
}

bool ngram_cmp::operator()(NGram first, NGram second)
{
	for(size_t i = 0; i<this->n_gram_size; i++)
	{
		int prevres;
		//Here we compare words in the order specified by word_order.
		if(!(prevres = strcmp((const char*) first.ngram[word_order[i]].string,(const char*) second.ngram[word_order[i]].string)))
		{
			continue;
		}else
		       return prevres < 0;
	}
	return false;
}

class Dict
{
	Dict(ngram_cmp cmp,unsigned int ngramsize);
	{
		this->ngramsize = ngramsize;
		innerMap = map<NGram, long long int, ngram_cmp>(cmp);
	}
	void addNGram(NGram &to_add)
	{
		pair<map<NGram,long long int>::iterator, bool> res = innerMap.insert(map<NGram, long long int>::value_type(to_add,1));
		if(!res.second)
		{
			res.first.second++;
		}
	}
	void writeToDisk( const char* prefix,unsigned int buffercount,uint8_t starting_character)
	{
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
	
		if(snprintf(buf,512,"./%s/0_%u/%u.out",prefix,buffercount,(unsigned int) starting_character) >= 512)
		{
			//This should never happen either.
			fprintf(stderr,"Error, bad coding");
			exit(-1);
		}
		FILE* outfile = fopen(buf,"w");
		if(!outfile)
		{
			fprintf(stderr,"Unable to open file %s", buf);
			exit(-1);
		}
	
		long long int freq;
		for (map<NGram, long long int>::const_iterator itr = innerMap.begin(); itr != innerMap.end(); itr++)
		{
			uint8_t* n_gram = itr->first.ngram;
			for(size_t j = 0; j< n_gram_size; j++)
			{
				ulc_fprintf(outfile, "%U", n_gram[word_order[j]].string);
				if(j != n_gram_size - 1)
				{
					fprintf(outfile," ");
				}
			}
			freq = itr->second;
			fprintf(outfile,"\t%lld\n", (long long int) freq);
		}
		fclose(outfile);
		return;
	}

	void clear()
	{
		innerMap.clear();
	}
	private:
		map<NGram, long long int,ngram_cmp> innerMap;
		unsigned int ngramsize;
}


void Buffer::add_word(uint8_t* word_location, int &mmgnt_retval)
{
	word* new_word;
	words_bottom -= sizeof(*new_word);
	new_word = (word*) ((internal_buffer + buffer_size) - words_bottom);

	if((words_bottom - strings_top) < maximum_single_allocation)
	{
		mmgnt_retval = -1;
	}

	last_word->next = new_word;
	new_word->prev = last_word;
	last_word = new_word;
	return;
}


uint8_t* Buffer::allocate_for_string(size_t numbytes, int &mmgnt_retval)
{
	strings_top += numbytes;
	uint8_t* result = (uint8_t*) (internal_buffer + strings_top - numbytes);
	if((words_bottom -  strings_top) < maximum_single_allocation)
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

void Buffer::set_starting_pointer(size_t nmeb)
{
	starting_pointer = nmeb * sizeof(word*);
}

void Buffer::set_ending_pointer(size_t nmeb)
	ending_pointer = buffer_size - nmeb * sizeof(*word);
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

IndexCollection::IndexCollection(size_t buffer_size,size_t maximum_single_allocation,size_t ngramsize,unsigned int wordsearch_index_upto)
{
	this->buffer_size = buffer_size;
	this->maximum_single_allocation = maximum_single_allocation;
	this->ngramsize = ngramsize;
	this->numcombos = number_of_special_combinations(wordsearch_index_upto);
	this->combinations.reserve(numcombos);
	this->prefixes.reserve(numcombos);
	this->mergeschedulers = calloc(sizeof(*this->mergeschedulers) * numcombos);
	if(!this->mergeschedulers)
	{
		cerr<<"Not enough memory, exiting"<<endl;
		exit(-1);
	}

	if(wordsearch_index_upto != 1)
	{
		 fprintf(stderr, "Generating %d indices\n",(int) numcombos);
	}


	 CombinationIterator combinations(wordsearch_index_upto);
	 size_t combo_number = 0;
	 do
	 {
 		unsigned int* current_combo = *combinations;		 
		unsigned int* new_word_combo = (unsigned int*) malloc(ngramsize * sizeof(new_word_combo));


		new_word_combo.reserve(ngramsize);

	 	size_t j = 0;
		for(size_t i = 0; i < wordsearch_index_upto;i++)
		{
			char buf[4];
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


		char* new_prefix = malloc(6 + (3 + 1) * ngramsize + 1);//this means 999-grams are the largest we can search for.
		strcpy(new_prefix,"tmp.by");
		char* ptr = new_prefix + strlen(new_prefix);
		for(size_t i = 0; i<ngramsize; i++)
		{
			ptr += sprintf(ptr,"_%d",(int) current_prefix[i]);
		}


		combinations.push_back(new_word_combo);
		prefixes.push_back(new_prefix);
		free(current_combo);
		combo_number++;

	 }while(++combinations);

	 if(combo_number != numcombos)
	 {
		fprintf(stderr,"There is an bug in index calculating\n");
		exit(-1);
	 }

	if(!first_internal_buffer)
	{
		cerr<<"Not enough memory"<<endl;
		exit(-1);
	}
	current_buffer = new Buffer(first_internal_buffer,buffer_size,maximum_single_allocation,&null_word);
}

void IndexCollection::newBuffer(unsigned int old_buffer_count,unsigned int rightmost_run)
{
	word* words_begin = current_buffer->words_begin();
	word* words_end = current_buffer->words_end();

	//This step will take a long time:
	std::sort(words_begin,words_end);

	vector<Dict> current_ngrams;
	current_ngram.reserve(ngramsize);

	for(size_t i = 0; i < numcombos; i++)
	{
		ngram_cmp cur(ngramsize,combinations[i]);
		current_ngrams.push(Dict(cur,ngramsize));
	}


	uint8_t *previous_word = words_begin->contents;
	NGram new_ngra;
	for(word* current_word = words_begin; current_word != words_end;)
	{
		do
		{
			if(current_word == words_end)
				break;

			current_word_str = current_word.contents;
			for(size_t i = 0; i < numcombos; i++)
			{
				new_ngram.ngram.clear();
				new_ngram.ngram.reserve(ngramsize); 
				int skip = 0;
				for(size_t j = 0; j < ngramsize; j++)
				{
					word* the_added_word;
					int offset = combinations[i][j] - combinations[i][0]; //TODO: optimize so that this only happens once.
					if(offset > 0)
					{
						the_added_word = current_word;
						for(;(the_added_word != &null_word) && offset;offset--)
						{
							the_added_word = the_added_word->next;
						}
					}else if(offset < 0)
					{
						the_added_word = current_word;
						for(;(the_added_word != &null_word) && offset;offset++)
						{
							the_added_word = the_added_word->prev;	
						}
					}else 
					{
						the_added_word = current_word;
					}

					if(the_added_word == &null_word)
					{
						skip = 1;
						break;
					}
					new_ngram.push_back(the_added_word.contents);
				}
				if(!skip)
				{
				}
				else
					skip = 0;
			}
			current_word++;
		}while(!strcmp(previous_word_str,current_word_str));
		//We now write out any n-grams that we have:
		for(size_t i = 0; i< numcombos; i++)
		{
			current_ngrams[i].writeToDisk(prefixes[i],buffercount,previous_word[i]);
			current_ngrams[i].clear();
		}	
	}
	//At this point do all merging
	for(int i = 0; i< numcombos; i++)
	{
		schedule_next_merge(0,buffercount,rightmost_run,mergeschedulers+i,prefixes[i]);
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
		char* finalpath = (char*) malloc(strlen(prefix[i]) * sizeof(*finalpath) + 1);
		strcpy(finalpath,prefix[i] + 4);//To remove preceding "tmp."
		recursively_remove_directory(finalpath);
		char* original_path = (char*) malloc(strlen(prefix[i]) + 256 );//More than enough
		sprintf(original_path, "%s/%d_0",prefix[i],k);
		rename(original_path,finalpath);
		recursively_remove_directory(prefix[i]);
		free(finalpath);
		free(original_path);
	}
	return;
}
