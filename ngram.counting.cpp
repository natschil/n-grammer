#include "ngram.counting.h"

using namespace std;

/*Some global variables*/

	//lexicon and prev_lexicon both are set to be pointers to an array of 256 map containers in analyze_ngrams()
	//(one for each starting byte of the n-gram)
	letterDict *lexicon = NULL;
	letterDict *prev_lexicon = NULL;

	//These two variables are also set at the start of analyze_ngrams()
	int n_gram_size = 0;
	int max_ngram_string_length = 0;

/* Function Declarations*/

	static void mark_ngram_occurance(myNGram new_ngram);
	static void swapDicts();
	static void* writeLetterDict(int buffercount,int number,letterDict &sublexicon);


	class word_list;
	int getnextngram(FILE* f,long long int &totalwords,uninorm_t n,word_list &my_n_words,myNGram &str);


/*myNGram comparison function is used by stl map*/

bool ngram_cmp::operator()(myNGram first, myNGram second)
{
	for(int i = 0; i<n_gram_size; i++)
	{
		int prevres;
		//We use strcmp here so that it is exactly the same as what the merger does.
		if(!(prevres = strcmp((const char*) first.ngram[i].string,(const char*) second.ngram[i].string)))
		{
			continue;
		}else
		       return prevres < 0;
	}
	return false;
}

/*The word_list class is used to store a list of the last n words encountered. Querying it's length is an O(1) operation, 
 * which makes it more efficient than the stl deque for that purpose*/
class word_list : public std::deque<myUString>
{
	private:
	int u8_characters_used; //The combined number of characters of all n strings in the n-gram.
	public:
	int size; //The number of words in this word_list

	word_list() : std::deque<myUString>()
	{
		this->size = 0;
		this->u8_characters_used = 0;
	}

	~word_list(){return;};

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
		this->u8_characters_used -= (this->front().length + 1);

		this->std::deque<myUString>::pop_front();
		return;
	}

	void push_back(myUString newstring)
	{
		this->size++;
		this->u8_characters_used += (newstring.length + 1);
		this->std::deque<myUString>::push_back(newstring);
	}

	//When switching buffers, some of the words in the word-list may still be in the old buffer.
	//This function copies them to the new buffer.
	void migrate_to_new_buffer()
	{
		int memmgnt_retval = 1;
		uint8_t* in_new_buffer = (uint8_t*) permanently_malloc(this->u8_characters_used * sizeof(*in_new_buffer),&memmgnt_retval);
		if(memmgnt_retval == -1)
		{
			//Because this function needs to be run right after a buffer switch, if nothing fits into the new buffer
			//It means that a buffer cannot hold a single n-gram
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

	//This function returns a myNGram object corresponding to the strings in the word-list.
	//It sets retval to -1 if permanently_malloc() does so.
	myNGram join_as_ngram(int *retval)
	{
		//What we end up returning
		myNGram result;

		//We allocate enough space for the n-gram.
		//The n-gram is stored as an array of pointers to the words it contains.
		result.ngram = (myUString*) permanently_malloc(sizeof(*result.ngram) * n_gram_size,retval);

		size_t j;
		word_list::iterator i;
		for(i = this->begin(), j = 0; i != this->end(); i++,j++)
		{
			result.ngram[j] = *i;
		}

		return result;
	}
};

/*Sets s to be a pointer to a (uint8_t, NUL terminated) string of the next word from f.
 * s is allocated using permenently_malloc, and normalized using norm.
 * Returns the size of f if there is a next word.
 * Returns 0 if there is no subsequent word in f. The value of s is undefined if this is the case.
 * Returns x so that x > MAX_WORD_SIZE if the contents of s, which are left undefined, should be ignored.
 * The following transformations of the input string from the file also take place:
 * 	Alphabetic characters from f are converted to their lowercase equivalents. (Note that this is done on a per-character basis)
 * 	Diacritics are left unmodified unless through normalization, or if they in the Unicode Sk category, in which case they are ignored.
 * 	Hyphen Characters are ignored unless they are within a word. 
 * 	Example: an input of --X-ray will result in the word x-ray being returned
 *	Punctuation characters are ignored.
 *	Any other character is also ignored.
 * Any whitespace constitutes a word delimiter.
 * memmanagement_retval should point to an integer which is set to -1 in the case of the buffer being switched.
 */


size_t getnextword(uint8_t* &s,FILE* f,uninorm_t norm,int *memmanagement_retval)
{

    size_t wordlength = 0; 
    uint8_t word[MAX_WORD_SIZE+1]; 

    //We read in unicode code points one at a time.
    wint_t wide_char;

    for(wordlength = 0;(wide_char = fgetwc(f)) != WEOF;)
    { 
	ucs4_t character = (ucs4_t)(wchar_t) wide_char;

	if(uc_is_property_white_space(character))
	{
		if(!wordlength) //We ignore preceding whitespace.
			continue;
		else //We have reached the end of the word
			break;
	}else if(wordlength <= MAX_WORD_SIZE) //Only read in characters while there is space to do so.
	{
		if(uc_is_property_alphabetic(character))
		{
			character = uc_tolower(character);

			size_t added_word_length = MAX_WORD_SIZE + 1 - wordlength;
			u32_to_u8(&character, 1, word + wordlength,&added_word_length);
			wordlength += added_word_length;

		}else if(uc_is_property_hyphen(character) && wordlength)
		{
			size_t added_word_length = MAX_WORD_SIZE + 1 - wordlength;
			u32_to_u8(&character,1,word+wordlength,&added_word_length);
			wordlength += added_word_length;

		}else if(wordlength && uc_is_property_diacritic(character) && !uc_is_general_category(character,UC_MODIFIER_SYMBOL))
		{
			size_t added_word_length = MAX_WORD_SIZE + 1 - wordlength;
			u32_to_u8(&character,1,word+wordlength, &added_word_length);
			wordlength += added_word_length;

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

    if(!strcmp((const char*) word,"END.OF.DOCUMENT---")) //---END.OF.DOCUMENT--- becomes END.OF.DOCUMENT--- because preceding hyphens are ignored.
    {
	return MAX_WORD_SIZE + 1;
    }

    if(wordlength)
    {
	//At this point we have an unnormalized lowercase unicode string.
	const size_t bytes_to_allocate = sizeof(*s) * (MAX_WORD_SIZE + 1);
	s = (uint8_t*) permanently_malloc(bytes_to_allocate,memmanagement_retval);
	size_t retval = bytes_to_allocate;

	u8_normalize(norm,word, wordlength, s, &retval);

	if((retval > MAX_WORD_SIZE)) //This is possible because we allow the normalizer to write up to MAX_WORD_SIZE + 1 characters
	{
		rewind_permanent_malloc(bytes_to_allocate);
		retval = MAX_WORD_SIZE + 1;
	}

	if((bytes_to_allocate- retval) > 0)
		rewind_permanent_malloc(bytes_to_allocate - retval - sizeof(*s)); //The last sizeof(*s) is so we don't rewind past the null byte

	s[retval] = (uint8_t) '\0';

	return retval;
   }else
   {
	return 0;
  }
}
/*This function is passed as a callback to init_permanent_malloc(), and is called directly after the buffer switch*/
static void swapDicts()
{
	//MARKER1
	#pragma omp flush
	letterDict *tmp = prev_lexicon;
	prev_lexicon = lexicon;
	lexicon = tmp;
	#pragma omp flush
	return;	

}

/* This function writes out sublexicon to 0_<buffercount>/i.out
 */
static void *writeLetterDict(int buffercount,int i,letterDict &sublexicon)
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
		myUString* n_gram = itr->first.ngram;
		for(int j = 0; j< n_gram_size; j++)
		{
			ulc_fprintf(outfile, "%U", n_gram[j].string);
			if(j != n_gram_size - 1)
			{
				fprintf(outfile," ");
			}
		}
		freq = itr->second;
		fprintf(outfile,"\t%lld\n", (long long int) freq);
	}
	fclose(outfile);
	//MARKER1 relies on the following step
	sublexicon.clear();
	return NULL;
}


/*Marks the occurance of an n-gram in the dictionary, or increments the counter if it already exists*/
static void mark_ngram_occurance(myNGram new_ngram)
{
	//We run a kind of bucket-sort which hopefully makes things slightly faster.
	size_t firstchar = (size_t) (uint8_t) new_ngram.ngram[0].string[0];

	letterDict &lD = lexicon[firstchar];

	//This is so that we can combine insertion with finding the object.
	std::pair<letterDict::iterator,bool> old = lD.insert(letterDict::value_type(new_ngram,1));
	if(!old.second) //The value was inserted.
	{
		old.first->second++;

	}
	return;
}

/*Writes the dictionary associated with the buffer page_group to disk*/
static void writeBufferToDisk(int buffercount,size_t page_group,letterDict* lexicon)
{
		setpagelock(page_group);
		fprintf(stderr,"Writing Buffer to disk\n");
		fflush(stderr);
		#pragma omp parallel for
		for(int i = 0; i< 256;i++)
		{
			writeLetterDict(buffercount,i,lexicon[i]);
		}
	
		#pragma omp flush
		unsetpagelock(page_group);
}


//Fills a buffer.
int fillABuffer(FILE* f, long long int &totalwords, uninorm_t norm, word_list &my_n_words,long long int &count)
{
	int page_group_at_start= current_page_group;
	//We get the lock for the current page group
	//Note that we do not have to worry about whether the other buffer is still being written to disk
	//because permanently_malloc() briefly gets the lock for the next page group once there is a buffer switch
	//This is required at MARKER3
	setpagelock(page_group_at_start);

	//Get whatever words are still in the old buffer and copy them to the new buffer
	//The old buffer has not been modified because nothing writes to it except fillABuffer
	my_n_words.migrate_to_new_buffer();

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
	//When the buffer is full, we switch buffers:
	switch_permanent_malloc_buffers();	

	unsetpagelock(page_group_at_start);
	return state;
}

long long int analyze_ngrams(unsigned int ngramsize,FILE* infile,FILE* outfile)
{
    //Initialization:
	n_gram_size = ngramsize;
	max_ngram_string_length = (ngramsize * (MAX_WORD_SIZE + 1)) + 1;
	#pragma omp flush(n_gram_size,max_ngram_string_length) //This is probably uneccessary

	init_merger(max_ngram_string_length);
	init_permanent_malloc(&swapDicts,max_ngram_string_length + n_gram_size*sizeof(myNGram::ngram));

	remove("./processing");
	mkdir("./processing",S_IRUSR | S_IWUSR | S_IXUSR);
	chdir("./processing");

	lexicon = new Dict;
	prev_lexicon = new Dict;

	long long int totalwords = 0;
	long long int count = 0;	
	word_list my_n_words;

	//We use this to normalize the unicode text. See http://unicode.org/reports/tr15/ for details on normalization forms.
	uninorm_t norm = UNINORM_NFKD; 
	setlocale(LC_CTYPE, "");


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
			//MARKER3:
			//fillABuffer has the following characteristics:
			//	a) When switching buffers, it waits for writeBufferToDisk to have finished with that buffer
			//	b) All the strings in a Dictionary come from the same buffer
			

			if(!state)
				break;
			if(state == -1)
			{
				state = 1;
				#pragma omp flush(buffercount)
				buffercount++;
				#pragma omp flush(buffercount)
				//We write the buffer to disk concurrently, and allow for the next buffer to be read in.
				//Due to the fact that writeBufferToDisk obtains the lock for the buffer it is writing to disk,
				//fillABuffer will always wait for the previous buffer to be written to disk, because 
				//permanently_malloc() does so at MARKER4:
				#pragma omp task firstprivate(buffercount,current_page_group,prev_lexicon) default(none)
				{

						writeBufferToDisk(buffercount,!current_page_group,prev_lexicon);	
						//Once we have finished writing the buffer to disk, we start the merging process:
						schedule_next_merge(0,buffercount,0);
						
				}
			}
			state = fillABuffer(infile,totalwords,norm,my_n_words,count);
			#pragma omp taskwait //This is so that only one buffer is written to disk at any one time

		}
		//Once we reach here we still have one buffer that needs to be written to disk.
		
		#pragma omp flush(buffercount)
		buffercount++;
		writeBufferToDisk(buffercount,current_page_group,prev_lexicon);//Because fillABuffer swapped the lexica
		schedule_next_merge(0,buffercount,1);//This sets of the last chain of merges.
	}
	}		
	//We've done all our reading from the input file.
	//We know that all merging is finished due to the implicit barrier at the end of the paralell section.
	
	int k = get_final_k();

	fprintf(stderr,"Merging subfiles\n");
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
		size_t wordlength = getnextword(word,f,n,&retval);
		
		if(!wordlength) //We've reached the end of the file, and we didn't read anything this time.
		{
			retval =  0;//It is safe to set retval here because even if there was a buffer switch from reading in words, it doesn't matter.
			break;
		}else if(wordlength > MAX_WORD_SIZE) //The word should not be counted as a word.
		{
			my_n_words.clear();
			continue;
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
		ngram = my_n_words.join_as_ngram(&retval);
		my_n_words.pop_front();
		break;
	}
}
		return retval;
}

