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

void DictCollection::writeToDisk(word* start)
{
	for(unsigned int i = 0; i< numcombos; i++)
	{
		dictionaries[i].writeToDisk(start);
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

	this->cmp = ngram_cmp(ngramsize,word_order,optimized_combo,null_word);

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
void Dict::writeToDisk(word* start)
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
			if(count)
				this->writeOutNGram(prev,count);
			count = 1;
			prev= i;
		}
	}

	if(count)
		this->writeOutNGram(prev, count);
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
