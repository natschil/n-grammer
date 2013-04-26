/*(c) 2013 Nathanael Schilling
 * This file contains functions used to deal with marking n-grams as they are found
 */

/*NGram comparison function which is used by stl map*/
#include "indices.h"
#include "searchindexcombinations.h"

static void *writeLetterDict(int buffercount,int i,letterDict &sublexicon,unsigned int n_gram_size,const char* prefix,const vector<unsigned int> &word_order);

ngram_cmp::ngram_cmp(unsigned int ngramsize,vector<unsigned int> &word_order)
{
	this->word_order = word_order;
	n_gram_size = ngramsize;
}

bool ngram_cmp::operator()(NGram *first, NGram *second)
{
	for(size_t i = 0; i<this->n_gram_size; i++)
	{
		int prevres;
		//Here we compare words in the order specified by word_order.
		if(!(prevres = strcmp((const char*) first->ngram[word_order[i]].string,(const char*) second->ngram[word_order[i]].string)))
		{
			continue;
		}else
		       return prevres < 0;
	}
	return false;
}
		


Index::Index(unsigned int ngramsize,vector<unsigned int> &combination,const char* prefix)
{
	 ngram_cmp compare(ngramsize,combination);
	 n_gram_size = ngramsize;
	 word_order = combination;

	 letters = vector<letterDict>(256);
	 for(size_t i = 0; i<256;i++)
	 {
		letters[i] = letterDict(compare);
	 }

	 this->prefix = prefix;

}


void Index::writeToDisk(int buffercount)
{
	for(int i = 0; i < 256;i++)
	{
		writeLetterDict(buffercount,i,letters[i],this->n_gram_size,prefix,word_order);
	}
	return;
} 



//Returns the (new) frequency if the ngram was already there
//Returns 0 otherwise.
long long int Index::mark_ngram_occurance(NGram* new_ngram)
{
	long long int retval = 0;
	new_ngram->num_occurances = 1;
	//We distribute the elements by their first character, which hopefully makes things slightly faster.
	size_t firstchar = (size_t) (uint8_t) new_ngram->ngram[word_order[0]].string[0];

	letterDict &lD = letters[firstchar];

	//The following method makes it possible to only lookup the element once when either inserting or incrementing the count.
	std::pair<letterDict::iterator,bool> old = lD.insert(letterDict::value_type(new_ngram,0));
	if(!old.second) //The value was inserted.
	{
		retval = ++old.first->first->num_occurances;
	}
	return retval;
}

IndexBufferPair::IndexBufferPair(unsigned int ngramsize, vector<unsigned int> &combination)
{
	word_order = combination;
	prefix = (char*) malloc(ngramsize*5 + strlen("tmp.by") + 1); //Assuming ngramsize fits into 4 decimal characters. MARKER5
	 strcpy(prefix,"tmp.by");

	 size_t size = combination.size();
	 char* ptr  = prefix + strlen(prefix);
	 for(size_t i = 0; i < size;i++)
	 {
		 *(ptr++) = '_';
		 ptr += sprintf(ptr,"%u",combination[i]);
	 }

	for(int i = 0; i < 2; i++)
	{
		buffers[i] = Index(ngramsize,word_order,prefix);
	}

	current_buffer = 0;
	memset(scheduling_table,0,sizeof(scheduling_table));

	return;

}

long long int IndexBufferPair::mark_ngram_occurance(NGram* new_ngram)
{
	return buffers[current_buffer].mark_ngram_occurance(new_ngram);
}

void IndexBufferPair::writeBufferToDisk(int buffer_num, int  buffercount,int is_finalbuffer)
{
	buffers[buffer_num].writeToDisk(buffercount);
	schedule_next_merge(0,buffercount,is_finalbuffer,&scheduling_table,prefix);
	return;
}

void IndexBufferPair::copyToFinalPlace(int k)
{
	//TODO: Copy any relevant metadata here...
	char* finalpath = (char*) malloc(strlen(prefix) * sizeof(*finalpath) + 1);
	strcpy(finalpath,prefix + 4);//To remove trailing "tmp.by"
	recursively_remove_directory(finalpath);
	char* original_path = (char*) malloc(strlen(prefix) + 256 );//More than enough
	sprintf(original_path, "%s/%d_0",prefix,k);
	rename(original_path,finalpath);
	recursively_remove_directory(prefix);
	return;
}

void IndexBufferPair::swapBuffers(void)
{
	//Due to MARKER6 we don't flush here.
	current_buffer = !current_buffer;
}

int IndexBufferPair::getCurrentBuffer(void)
{
	#pragma omp flush
	return current_buffer;
}

const char* IndexBufferPair::getPrefix(void)
{
	return (const char*) prefix;
}

unsigned long long int  factorial(unsigned int number)
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
unsigned long long number_of_special_combinations(unsigned int number)
{
	unsigned long long result = factorial(number)/
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

IndexCollection::IndexCollection(unsigned int ngramsize,unsigned int wordsearch_index_upto)
{
	 n_gram_size = ngramsize;
	 max_freq = 0;
	 if(wordsearch_index_upto > MAX_INDICES)
	 {
		 fprintf(stderr,"About to generate too many indices. Change the value MAX_INDICES in config.h if you want to allow this\n");
		 exit(-1);
	 }
	 unsigned long long numcombos = number_of_special_combinations(wordsearch_index_upto);

	 indices = vector<IndexBufferPair>(numcombos);
	 //indices.reserve(numcombos);

	 if(wordsearch_index_upto != 1)
	 {
		 fprintf(stderr, "Generating %d indices\n",(int) numcombos);
	 }


	 CombinationIterator combinations(wordsearch_index_upto);
	 size_t combo_number = 0;
	 do
	 {
 		unsigned int* current_combo = *combinations;		 
	 	vector<unsigned int> new_word_combo(n_gram_size);

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
					new_word_combo[j] = (unsigned int) k;		
					j++;
				}
			}
		}

		indices[combo_number] =  IndexBufferPair(ngramsize,new_word_combo);
		free(current_combo);
		combo_number++;

	 }while(++combinations);

	 if(combo_number != numcombos)
	 {
		fprintf(stderr,"There is an bug in index calculating\n");
		exit(-1);
	 }

	 indices_size = combo_number;//Since this doesn't change throughout.

	 return;

}

void IndexCollection::mark_ngram_occurance(NGram* new_ngram)
{
	long long int this_freq = 0;
	if(!(this_freq = indices[0].mark_ngram_occurance(new_ngram)))
	{
		//If the ngram is new, it needs to be added to all indices. Otherwise incrementing the first one
		//increments them all.
		for(size_t i = 1; i < indices_size ;i++)
		{
			indices[i].mark_ngram_occurance(new_ngram);
		}
	}
	if(this_freq > max_freq)
		max_freq = this_freq;

}

void IndexCollection::writeBufferToDisk(int buffer_num,int buffercount,int isfinalbuffer)
{
	for(size_t i = 0; i< indices_size;i++)
	{
		indices[i].writeBufferToDisk(buffer_num,buffercount,isfinalbuffer);
	}
}

void IndexCollection::copyToFinalPlace(int k)
{
	for(size_t i = 0; i < indices_size;i++)
	{
		indices[i].copyToFinalPlace(k);
	}
}

void IndexCollection::swapBuffers(void)
{
	//MARKER6: we flush here	
	#pragma omp flush
	for(size_t i= 0; i< indices_size;i++)
	{
		indices[i].swapBuffers();
	}
	#pragma omp flush
	return;
}

int IndexCollection::getCurrentBuffer(void)
{
	return indices[0].getCurrentBuffer();
}

void IndexCollection::writeMetadata(FILE* metadata_file)
{
	fprintf(metadata_file,"Indexes:\n");
	for(size_t i = 0; i< indices_size;i++)
	{
		fprintf(metadata_file,"\t%s\n",indices[i].getPrefix() + 4);
	}
	fprintf(metadata_file,"MaxFrequency:\t%lld\n",max_freq);
	return;
}


/* This function writes out sublexicon to <prefix>/0_<buffercount>/i.out
 * Note that <prefix> does not have to exist, but it may not exists as a non-directory.
 */
static void *writeLetterDict(int buffercount,int i,letterDict &sublexicon,unsigned int n_gram_size,const char* prefix,const vector<unsigned int> &word_order)
{
	if(mkdir(prefix,S_IRUSR | S_IWUSR | S_IXUSR) && (errno != EEXIST))
	{
		fprintf(stderr, "Unable to create directory %s",prefix);
	}

	char buf[512];//512 Characters should be more than enough to hold 2 integers, a few characters and prefix.

	if(snprintf(buf,512,"./%s/0_%d",prefix,buffercount) >= 512)
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

	if(snprintf(buf,512,"./%s/0_%d/%d.out",prefix,buffercount,i) >= 512)
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
	for (letterDict::iterator itr = sublexicon.begin(); itr != sublexicon.end(); itr++)
	{
		myUString* n_gram = itr->first->ngram;
		for(size_t j = 0; j< n_gram_size; j++)
		{
			ulc_fprintf(outfile, "%U", n_gram[word_order[j]].string);
			if(j != n_gram_size - 1)
			{
				fprintf(outfile," ");
			}
		}
		freq = itr->first->num_occurances;
		fprintf(outfile,"\t%lld\n", (long long int) freq);
	}
	fclose(outfile);
	//MARKER1 relies on the following step
	sublexicon.clear();
	return NULL;
}


