/*(c) 2013 Nathanael Schilling
 * This file contains functions used to deal with marking n-grams as they are found
 */

/*NGram comparison function which is used by stl map*/
#include "indices.h"


static void *writeLetterDict(int buffercount,int i,letterDict &sublexicon,int n_gram_size);

bool ngram_cmp::operator()(NGram *first, NGram *second)
{
	for(size_t i = 0; i<this->n_gram_size; i++)
	{
		int prevres;
		//We compare the words in exactly the same way as done by the merger, which is required by the merger.
		if(!(prevres = strcmp((const char*) first->ngram[i].string,(const char*) second->ngram[i].string)))
		{
			continue;
		}else
		       return prevres < 0;
	}
	return false;
}
		


Index::Index(size_t ngramsize)
{
	 n_gram_size = ngramsize;
	 ngram_cmp compare(ngramsize);
	 letters = vector<letterDict>(256);
	 for(int i = 0; i<256;i++)
	 {
		letters[i] = letterDict(compare);
	 }
	return;
}


void Index::writeToDisk(int buffercount)
{
	#pragma omp parallel for
	for(int i = 0; i < 256;i++)
	{
		writeLetterDict(buffercount,i,letters[i],this->n_gram_size);
	}
	return;
} 

void Index::mark_ngram_occurance(NGram* new_ngram)
{
	new_ngram->num_occurances = 1;
	//We distribute the elements by their first character, which hopefully makes things slightly faster.
	size_t firstchar = (size_t) (uint8_t) new_ngram->ngram[0].string[0];

	letterDict &lD = letters[firstchar];

	//The following method makes it possible to only lookup the element once when either inserting or incrementing the count.
	std::pair<letterDict::iterator,bool> old = lD.insert(letterDict::value_type(new_ngram,0));
	if(!old.second) //The value was inserted.
	{
		old.first->first->num_occurances++;
	}
	return;
}

IndexCollection::IndexCollection(size_t ngramsize)
{
	 n_gram_size = ngramsize;
	 indeces = vector<Index>(1);
	 for(int i = 0; i< 1;i++)
	 {
		 indeces[i] = Index(ngramsize);
	 }	
}

void IndexCollection::mark_ngram_occurance(NGram* new_ngram)
{
	indeces[0].mark_ngram_occurance(new_ngram);
}

void IndexCollection::writeToDisk(int buffercount)
{
	#pragma omp parallel for
	for(int i = 0; i< 1;i++)
	{
		indeces[i].writeToDisk(buffercount);
	}
}

IndexCollectionBufferPair::IndexCollectionBufferPair(size_t ngramsize)
{
	current_buffer = 0;
	n_gram_size = ngramsize;
	for(size_t i = 0; i<2;i++)
	{
		buffers[i] = IndexCollection(ngramsize);
	}
	return;
}

void IndexCollectionBufferPair::swapBuffers(void)
{
	 
	#pragma omp flush
	this->current_buffer = !this->current_buffer;
	#pragma omp flush
	return;
}

void IndexCollectionBufferPair::mark_ngram_occurance(NGram* new_ngram)
{
	buffers[current_buffer].mark_ngram_occurance(new_ngram);
	return;
}

/* This function writes out sublexicon to 0_<buffercount>/i.out
 */
static void *writeLetterDict(int buffercount,int i,letterDict &sublexicon,int n_gram_size)
{
	char buf[256];//256 Characters should be more than enough to hold 2 integers and a few characters.
	if(snprintf(buf,256,"./0_%d",buffercount) >= 256)
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

	if(snprintf(buf,256,"./0_%d/%d.out",buffercount,i) >= 256)
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
		for(int j = 0; j< n_gram_size; j++)
		{
			ulc_fprintf(outfile, "%U", n_gram[j].string);
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


void IndexCollectionBufferPair::writeBufferToDisk(size_t buffer_num,size_t buffercount)
{
	buffers[buffer_num].writeToDisk(buffercount);
	return;
}
