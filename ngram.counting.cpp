#include "ngram.counting.h"

using namespace std;

//Some global variables
letterDict *lexicon = NULL;
letterDict *prev_lexicon = NULL;
int n_gram_size = 0;
int max_ngram_string_length = 0;

//Some function declarations.
static void mark_ngram_occurance(myNGram new_ngram);
static void swapDicts();
static void* writeLetterDict(int buffercount,int number,letterDict* old_dict);
void writeLetterDictToFile(letterDict &sublexicon,FILE* outfile);
struct word_list;

int getnextngram(FILE* f,long long int &totalwords,uninorm_t n,word_list &my_n_words,myNGram &str);

bool ngram_cmp::operator()(myNGram first, myNGram second)
{
	for(int i = 0; i<n_gram_size; i++)
	{
		int prevres;
		//if(!(prevres = (u8_cmp2(first.ngram[i].string,first.ngram[i].length,second.ngram[i].string,second.ngram[i].length))))//Makes merging easier because we don't need to convert to UTF16 again..
		if(!(prevres = strcmp((const char*) first.ngram[i].string,(const char*) second.ngram[i].string)))
		{
			continue;
		}else
		       return prevres < 0;
	}
	return false;
}

struct word_list : public std::deque<myUString>
{
	word_list() : std::deque<myUString>()
	{
		this->size = 0;
		this->u8_characters_used = 0;
	}

	~word_list() 
	{
	}
	void clear()
	{
		while(this->size)
		{
			this->pop_front();
		}
		this->u8_characters_used = 0;
		return;
	}

	void pop_front()
	{
		this->size--;
		myUString old_string = this->front();
		this->u8_characters_used -= (old_string.length + 1);
		this->std::deque<myUString>::pop_front();
		return;
	}

	void push_back(myUString newstring)
	{
		this->size++;
		this->u8_characters_used += (newstring.length + 1);
		this->std::deque<myUString>::push_back(newstring);
	}

	//This function copies everything in the word list to a new buffer.
	void migrate_to_new_buffer()
	{
		int memmgnt_retval = 1;
		uint8_t* in_new_buffer = (uint8_t*) permanently_malloc(this->u8_characters_used * sizeof(uint8_t),&memmgnt_retval);
		if(memmgnt_retval == -1)
		{
			fprintf(stderr,"Buffer size too small. Please reconfigure, recompile and rerun\n");
			exit(-1);
		}
		uint8_t* ptr = in_new_buffer;
		for(word_list::iterator i = this->begin(); i != this->end(); i++)
		{
			myUString cur = *i;
			memcpy(ptr,cur.string,(cur.length + 1)*sizeof(*cur.string));
			(*i).string = ptr;
			ptr += (cur.length + 1);
		}
	}

	//This function ignores the contents of *retval
	myNGram join_as_ngram(int *retval)
	{
		//We need a separate retval because at MARKER2 we rely on the fact that this function will only *change*
		//retval if the buffer switch happens from this function, but ignore it if retval indicates that the buffer 
		//was previously switched.
		int memory_management_retval = 1;
		//What we end up returning
		myNGram result;
		//We allocate enough space for the pointers to the words
		result.ngram = (myUString*) permanently_malloc(sizeof(*result.ngram) * n_gram_size,&memory_management_retval);
		//If it didn't all fit in the same buffer, we make sure it is all in the same buffer.
		if(memory_management_retval == -1)
		{
			this->migrate_to_new_buffer();
			*retval = -1;
		}

		//Two iterators
		size_t j;
		word_list::iterator i;

		//The n-gram is stored as an array of pointers to the words it contains.
		for(i = this->begin(), j = 0; i != this->end(); i++,j++)
		{
			result.ngram[j] = *i;
		}
		//We return the n-gram
		return result;
	}
	int size;
	private:
	int u8_characters_used;
};

//Sets s to point to a normalized uint8_t* with the next word from f. s is allocated using permenently_malloc, and normalized using n.
//Returns the size of f iff there is a next word.
//Returns 0 if there is no subsequent word in f. The value of s is undefined if this is the case.
//Returns x so that x > MAX_WORD_SIZE if the contents of s, which are left undefined, should be ignored.
//Alphabetic characters from f are converted to their lowercase equivalents.
//Diacritics are left unmodified unless through normalization, or if they in the Unicode Sk category
//Hyphen Characters are ignored unless they are within a word. Therefore an input of --X-ray will result in the word x-ray being returned
//Punctuation and Digit Characters are Ingored
//Any whitespace constitutes a word delimiter.
//Any other characters are ignored.


int getnextword(uint8_t* &s,FILE* f,uninorm_t norm,int *memmanagement_retval)
{

    int wordlength = 0; 
    int first_is_hyphen = 0;
    uint8_t word[MAX_WORD_SIZE+1]; //words may not be longer than MAX_WORD_SIZE characters.
    //The size is MAX_WORD_SIZE + 1 so that it can hold a word of length MAX_WORD_SIZE+1 which is longer than MAX_WORD_SIZE characters and will hence trigger it to be ignored in 
    //The next code.
    wint_t wide_char;

    //See http://icu-project.org/apiref/icu4c/ustdio_8h.html#a48b9be06c611643848639fa4c22a16f4
    for(wordlength = 0;((wide_char = fgetwc(f)) != WEOF);)
    { 
	ucs4_t character = (ucs4_t)(wchar_t) wide_char;
	//We now check whether or not the character is a whitespace character:
	if(uc_is_property_white_space(character))
	{
		if(!wordlength) //We ignnore preceding whitespace.
			continue;
		else //We have reached the end of the word
			break;
	}else if(wordlength <= MAX_WORD_SIZE) //After 41 characters we ignore all non-whitespace stuff.
	{
		if(uc_is_property_alphabetic(character))
		{
			character = uc_tolower(character);
			size_t added_word_length = MAX_WORD_SIZE + 1 - wordlength;
			u32_to_u8(&character, 1, word + wordlength,&added_word_length);
			wordlength+= added_word_length;
		}else if(uc_is_property_hyphen(character) || 
				(uc_is_property_diacritic(character) && !uc_is_general_category(character,UC_MODIFIER_SYMBOL)))
		{

			if(uc_is_property_hyphen(character))
			{
				first_is_hyphen = 1;	
			}
			if(wordlength)
			{
				//We treat hyphens like we treat normal characters..
				size_t added_word_length = MAX_WORD_SIZE + 1 - wordlength;
				u32_to_u8(&character,1,word+wordlength, &added_word_length);
				wordlength += added_word_length;
			}
		}else if(uc_is_property_punctuation(character))
		{
			continue;
		}else
		{
			continue;
		}
	}
    }	
    if(wordlength > MAX_WORD_SIZE)
    {
	    return MAX_WORD_SIZE + 1;
    }
    word[wordlength] = '\0';
    if(first_is_hyphen && !strcmp((const char*) word,"END.OF.DOCUMENT"))
    {
	return MAX_WORD_SIZE + 1;
    }

    if(wordlength)
    {
	//At this point we have a unicode string. However, normalization issues are still present
	const size_t allocated_buffer_size = sizeof(*s) * (MAX_WORD_SIZE + 1);
	s = (uint8_t*) permanently_malloc(allocated_buffer_size,memmanagement_retval);
	size_t retval = allocated_buffer_size;

	u8_normalize(norm,word, wordlength, s, &retval);
	if((retval > MAX_WORD_SIZE))
	{
		rewind_permanent_malloc(sizeof(*s) * (MAX_WORD_SIZE + 1));	
		retval = MAX_WORD_SIZE + 1;
	}
	if((allocated_buffer_size - retval) > 0)
		rewind_permanent_malloc(allocated_buffer_size - retval - sizeof(*s)); //The last sizeof(*s) is so we don't rewind past the null byte
	s[retval] = (uint8_t) '\0';
	return retval;
   }else
   {
	return 0;
  }
}

static void swapDicts()
{
	//MARKER1
	letterDict *tmp = prev_lexicon;
	prev_lexicon = lexicon;
	lexicon = tmp;
	#pragma omp flush
	return;	

}


static void *writeLetterDict(int buffercount,int i,letterDict* old_dict)
{
	char buf[256];//Big enough.
	if(snprintf(buf,256,"./0_%d",buffercount) >= 256)
	{
		//This should never happen.
		fprintf(stderr,"Error, poor program design");
		exit(-1);
	}
	if(mkdir(buf,S_IRUSR | S_IWUSR | S_IXUSR ) && (errno !=  EEXIST))
	{
		//This should also never happen
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

	writeLetterDictToFile(old_dict[i],outfile);
	fclose(outfile);
	//MARKER1 relies on the following step
	old_dict[i].clear();
	return NULL;
}


void writeLetterDictToFile(letterDict &D,FILE* outfile)
{
	long long int freq;

	for (letterDict::iterator i = D.begin(); i != D.end(); i++) {
		myUString* n_gram = i->first.ngram;
		for(int j = 0; j< n_gram_size; j++)
		{
			ulc_fprintf(outfile, "%U", n_gram[j].string);
			if(j != n_gram_size - 1)
			{
				fprintf(outfile," ");
			}
		}
		freq = i->second;
		fprintf(outfile,"\t%lld\n", (long long int) freq);
	}
}


static void mark_ngram_occurance(myNGram new_ngram)
{
	
	//We run a kind of bucket-sort which hopefully makes things slightly faster.
	size_t firstchar = (size_t) (unsigned char) new_ngram.ngram[0].string[0];
	letterDict &lD = lexicon[firstchar];

	std::pair<letterDict::iterator,bool> old = lD.insert(letterDict::value_type(new_ngram,1));
	if(!old.second) //The value was inserted.
	{
		old.first->second++;

	}

	return;
}

static void writeBufferToDisk(int buffercount,size_t page_group,letterDict* lexicon)
{
		setpagelock(page_group);
		fprintf(stderr,"Switching buffers\n");
		#pragma omp parallel for
		for(int i = 0; i< 256;i++)
		{
			writeLetterDict(buffercount,i,lexicon);
		}
	
		#pragma omp flush
		unsetpagelock(page_group);
}


int fillABuffer(FILE* f, long long int &totalwords, uninorm_t norm, word_list &my_n_words,long long int &count)
{
	int state = 1;
	myNGram currentngram;
	while(state == 1)
	{


		state = getnextngram(f,totalwords,norm,my_n_words,currentngram);
		if(state == 0)
			break;
		mark_ngram_occurance(currentngram);

		count++;
		if((count % 1000000 == 0))
		{
			fprintf(stderr,"%lld\n",(long long int) count);
		}

	}
	return state;
}

long long int analyze_ngrams(unsigned int ngramsize,FILE* infile,FILE* outfile)
{
    //Initialization:
	n_gram_size = ngramsize;
	#pragma omp flush(n_gram_size)
	max_ngram_string_length = (ngramsize * (MAX_WORD_SIZE + 1)) + 1;
	init_merger(max_ngram_string_length);
	init_permanent_malloc(&swapDicts);

	remove("./processing");
	mkdir("./processing",S_IRUSR | S_IWUSR | S_IXUSR);
	chdir("./processing");
	lexicon = new Dict;
	prev_lexicon = new Dict;
	long long int totalwords = 0;
	word_list my_n_words;
	long long int count = 0;	
	//We use this to normalize the unicode text. See http://unicode.org/reports/tr15/ for details.
	uninorm_t norm = UNINORM_NFKD; 
	setlocale(LC_CTYPE, "");


    //Stage 1: We get n-grams and mark them in paralell

	int state = 1;
	volatile int buffercount = -1;
	#pragma omp parallel 
	{
	#pragma omp master
	{
       		//shared(nextstring,infile,norm,state,currentstring,stderr,count,totalwords,buffercount,current_page_group,my_n_words)
		//Here we fill one half of the memory used with n-grams.
		while(1)
		{
			#pragma omp flush(state)
			if(!state)
				break;
			if(state == -1)
			{
				state = 1;
				#pragma omp flush(buffercount)
				buffercount++;
				#pragma omp flush(buffercount)
				#pragma omp task firstprivate(buffercount,current_page_group,prev_lexicon) default(none)
				{

						writeBufferToDisk(buffercount,!current_page_group,prev_lexicon);	
						//Once we have finished writing the buffer to disk, we start the merging process:
						schedule_next_merge(0,buffercount,0);
				}
			}
			#pragma omp taskwait
			state = fillABuffer(infile,totalwords,norm,my_n_words,count);
		}
		//Once we reach here we still have one buffer that needs to be written to disk.
		#pragma omp flush(buffercount)
		buffercount++;
		writeBufferToDisk(buffercount,current_page_group,lexicon);
		schedule_next_merge(0,buffercount,1);
	}
	}		
	//We've done all our reading from the input file.
	//We know that all merging is finished due to the implicit barrier at the end of the paralell section.
	
	int k = get_final_k();

	fprintf(stderr,"Mergeing subfiles\n");
	//At this point we have 256 input files that can be combined.
	for(int i = 0; i< 256; i++)
	{
		char buf[256];
		snprintf(buf,256, "./%d_0/%d.out",k,i);
		FILE* cur = fopen(buf,"r");
		if(!cur)
		{
			fprintf(stderr,"The programmer has made an error, this should never happen. Could not open file %s\n",buf);
			exit(-1);
		}
		int curchar;
		while((curchar = fgetc(cur)) != EOF)
		{
			fputc((char) curchar,outfile);
		}
	}
	free_all_pages();
	return totalwords;
}

//Returns 1 for simpl having fetched the next n-gram
//Returns 0 if there is nothing more to fetch
//Returns -1 if we need to switch buffers.
int getnextngram(FILE* f,long long int &totalwords,uninorm_t n,word_list &my_n_words,myNGram &ngram)
{
	int retval;
#pragma omp critical
{
	uint8_t* word;
	retval = 1;
	while(true)
	{
		int wordlength = getnextword(word,f,n,&retval);
		
		if(!wordlength) //We've reached the end of the file, and we didn't read anything this time.
		{
			retval =  0;//It is safe to set retval here because even if there was a buffer switch from reading in words, it doesn't matter.
			break;
		}else if(wordlength > MAX_WORD_SIZE) //The word should not be counted as a word.
		{
			my_n_words.clear();
			continue;
		}
		if(retval == -1) //I.e. the previous buffer is full
		{
			my_n_words.migrate_to_new_buffer(); //We migrate what we have so far the buffer that the current word was written to.
		}

		//We add the word to our list.
		totalwords++;
		myUString newUString;
		newUString.string = word;
		newUString.length = wordlength;
		my_n_words.push_back(newUString);

		//If we don't have enough words for an n-gram, we get the next word.
		if(my_n_words.size < n_gram_size)
			continue;
	
		//If we do however have enough words for an n-gram, we write it out.
		//MARKER2: We can pass retval in here even if it was changed by getnextword because join_as_ngram() ignores the value of retval
		ngram = my_n_words.join_as_ngram(&retval);
		my_n_words.pop_front();
		break;
	}
}
		return retval;
}

