#include "ngram.counting.h"

using namespace std;

/*Some global variables*/

	//These two global variables are set the start of count_ngrams(), and are not excpected to change after that
	int n_gram_size = 0;
	int max_ngram_string_length = 0;

/* Function Declarations*/

	class word_list;
	int getnextngram(FILE* f,long long int &totalwords,uninorm_t n,word_list &my_n_words,NGram *&str);




/*The word_list class is used to store a list of the last n words encountered. Querying it's length is an O(1) operation, 
 * which makes it more efficient than the stl deque for that purpose
 */
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

	//This function returns a NGram object corresponding to the strings in the word-list.
	//It sets retval to -1 if permanently_malloc() does so.
	NGram* join_as_ngram(int *retval)
	{
		//What we end up returning

		//We allocate enough space for the n-gram.
		//The n-gram is stored as an array of pointers to the words it contains.
		NGram* result = (NGram*)permanently_malloc(sizeof(*result) + (sizeof(*result->ngram) *n_gram_size),retval);

		size_t j;
		word_list::iterator i;
		for(i = this->begin(), j = 0; i != this->end(); i++,j++)
		{
			result->ngram[j] = *i;
		}

		return result;
	}
};


//Returns 0 if there is no next character.
//See http://tools.ietf.org/html/rfc3629 Section 3
int get_next_ucs4t_from_file(FILE* f,ucs4_t *character)
{
start:
	uint8_t buf[4];
	int first = fgetc(f);
	if(first == EOF)
		return 0;

	buf[0] = first;

afterfirstcharacter:

	if(!(first & (1 << 7)))
	{
		if(u8_mbtouc(character,buf,4) == -1)
		{
			goto start;
		}else
			return 1;
	}else if(first &(1 << 6)  )
	{
		if(!(first &( 1 <<  5)))//There are two characters
		{
			int second = fgetc(f);
			if(second == EOF)
				return 0;
			buf[1] = second;
			if(u8_mbtouc(character,buf,2) == -1)
			{
				buf[0] = second; //Maybe simply the first byte was wrong.
				goto afterfirstcharacter;
			}else return 1;
		}else if(!(first & ( 1 << 4) )) //There are three characters:
		{
			int second = fgetc(f);
			if(second == EOF)
				return 0;
			if((second >> 6) != 2) //Invalid character here already.
			{
				buf[0] = second;	
				goto afterfirstcharacter;
			}
			int third = fgetc(f);
			if(third == EOF)
				return 0;

			buf[1] = second;
			buf[2] = third;
			if(u8_mbtouc(character,buf,3) == -1)
			{
				buf[0] = third;
				goto afterfirstcharacter;
			}else
				return 1;
		}else if(!(first  & ( 1 << 3)))
		{
			int second = fgetc(f);
			if(second == EOF)
				return 0;
			if((second >> 6) != 2)
			{
				buf[0] = second;
				goto afterfirstcharacter;
			}

			int third = fgetc(f);
			if(third == EOF)
				return 0;
			if((third >> 6) != 2)
			{
				buf[0] = third;
				goto afterfirstcharacter;
			}
			int fourth = fgetc(f);
			if(fourth == EOF)
				return 0;


			buf[1] = second;
			buf[2] = third;
			buf[3] = fourth;
			if(u8_mbtouc(character,buf,3) == -1)
			{
				buf[0] = fourth;
				goto afterfirstcharacter;
			}else
				return 1;
		}else
			goto start;
	}else
		goto start;

}

/*Sets s to be a pointer to a (uint8_t, NUL terminated) string of the next word from f.
 * s is allocated using permenently_malloc, and normalized using norm.
 *
 * Returns the size of f if there is a next word.
 * Returns 0 if there is no subsequent word in f. The value of s is undefined if this is the case.
 * Returns x so that x > MAX_WORD_SIZE if the contents of s, which are left undefined, should be ignored.
 *
 * The following transformations of the input string from the file also take place:
 * 	Alphabetic characters from f are converted to their lowercase equivalents. (Note that this is done on a per-character basis)
 * 	Diacritics are left unmodified unless through normalization, or if they in the Unicode Sk category, in which case they are ignored.
 * 		Currently Diacritics which are not within a word are ignored.
 * 	Hyphen Characters are ignored unless they are within a word. 
 * 		For example: an input of --X-ray will result in the word x-ray being returned
 *	Punctuation characters are ignored.
 *	Any other character is also ignored.
 *
 * Any whitespace constitutes a word delimiter.
 * memmanagement_retval should point to an integer: *memmanagement_retval is set to -1 if the buffer should be switched as soon as possible.
 */


size_t getnextword(uint8_t* &s,FILE* f,uninorm_t norm,int *memmanagement_retval)
{

    size_t wordlength = 0; 
    uint8_t word[MAX_WORD_SIZE+1]; 


    ucs4_t character;
    for(; get_next_ucs4t_from_file(f,&character);)
    { 
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
		rewind_permanent_malloc(bytes_to_allocate - retval - sizeof(*s)); //The last sizeof(*s) is for the NUL at the end of the string.

	s[retval] = (uint8_t) '\0';

	return retval;
   }else
   {
	return 0;
  }
}

//Returns 1 for simply having fetched the next n-gram
//Returns 0 if there is nothing more to fetch
//Returns -1 if we need to switch buffers.

int getnextngram(FILE* f,long long int &totalwords,uninorm_t n,word_list &my_n_words,NGram *&ngram)
{
	int retval;
	uint8_t* word;
	retval = 1;
	while(true)
	{
		size_t wordlength = getnextword(word,f,n,&retval);
		
		if(!wordlength) //We've reached the end of the file.
		{
			retval =  0;
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

		my_n_words.pop_front(); //So that the word-list does not grow infinitely
		break;
	}
		return retval;
}




/*Writes the dictionary associated with the buffer page_group to disk*/
static void writeBufferToDisk(int buffercount,size_t page_group,IndexCollectionBufferPair &index_collection,size_t buffer_num,int isfinalbuffer)
{
		setpagelock(page_group);
		fprintf(stderr,"Writing Buffer %d to disk\n",buffercount);
		fflush(stderr);
		index_collection.writeBufferToDisk(buffer_num,buffercount,isfinalbuffer);
		#pragma omp flush
		unsetpagelock(page_group);
}


//Fills a buffer with words and n-grams.
int fillABuffer(FILE* f, long long int &totalwords, uninorm_t norm, word_list &my_n_words,long long int &count,IndexCollectionBufferPair &indeces)
{

	//We get the lock for the current page group.
	//If anything else still has the lock for it, this function will wait for that to finish.
	setpagelock(current_page_group);

	//Get whatever words are still in the old buffer and copy them to the new buffer
	//The old buffer has not been modified because nothing writes to the buffers except fillABuffer
	my_n_words.migrate_to_new_buffer();

	int state = 1;
	int first = 1;
	NGram *currentngram;
	NGram *previousngram;
	while(state == 1)
	{

		if(!first)
		{
			#pragma omp task shared(previousngram,indeces) default(none)
			{
				indeces.mark_ngram_occurance(previousngram);
			}
		}

		#pragma omp task shared(state,f,totalwords,norm,my_n_words,currentngram)
		state = getnextngram(f,totalwords,norm,my_n_words,currentngram);

		#pragma omp taskwait
		#pragma omp flush(currentngram)
		previousngram = currentngram;
		#pragma omp flush(previousngram)
		if(state == 0)
			break;
		count++;
		if((count % 1000000 == 0))
		{
			fprintf(stderr,"%lld\n",(long long int) count);
		}
		first = 0;
	}
	if(state == -1)
		indeces.mark_ngram_occurance(previousngram);

	//When the buffer almost full, we switch buffers:
	switch_permanent_malloc_buffers();	
	indeces.swapBuffers();

	unsetpagelock(!current_page_group);

	return state;
}

long long int count_ngrams(unsigned int ngramsize,FILE* infile,const char* outdir,unsigned int wordsearch_index_depth)
{
    //Initialization:
    	struct timeval start_time,end_time;
  	gettimeofday(&start_time,NULL);

	n_gram_size = ngramsize;
	max_ngram_string_length = (ngramsize * (MAX_WORD_SIZE + 1)) + 1;
	#pragma omp flush(n_gram_size,max_ngram_string_length) //This is probably uneccessary

	init_merger(max_ngram_string_length);
	init_permanent_malloc(max_ngram_string_length + n_gram_size*sizeof(*(NGram::ngram)) + sizeof(NGram));

	mkdir(outdir,S_IRUSR | S_IWUSR | S_IXUSR);
	chdir(outdir);

	long long int totalwords = 0;
	long long int count = 0;	
	word_list my_n_words;

	IndexCollectionBufferPair *final_indices = new IndexCollectionBufferPair(ngramsize,wordsearch_index_depth);

	//We use this to normalize the unicode text. See http://unicode.org/reports/tr15/ for details on normalization forms.
	uninorm_t norm = UNINORM_NFKD; 
	setlocale(LC_CTYPE, "");

    //Here we fill the buffers, write them to disk and merge them.
	int state = 1; //state is 0 when the file is finished, -1 if the buffer has just been switched and 1 otherwise.
	volatile int buffercount = -1;

	#pragma omp parallel 
	{
	#pragma omp master
	{
		//Here we fill one half of the memory used with n-grams.
		while(1)
		{
			
			//fillABuffer has the following characteristics:
			//	a) When switching buffers, it waits for writeBufferToDisk to have finished with that buffer
			//	b) All the strings and n-grams in a Dictionary come from the same buffer
			
			#pragma omp flush(state)
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
				size_t previous_buffer = !final_indices->get_current_buffer();
				#pragma omp task firstprivate(buffercount,current_page_group,previous_buffer) shared(final_indices) default(none)
				{

						//Note that writeBufferToDisk starts the merging process once a buffer has been written to disk.
						writeBufferToDisk(buffercount,!current_page_group,*final_indices,previous_buffer,0);
						
				}
			}
			int newstate = 0;
			#pragma omp task shared(infile,totalwords,norm, my_n_words,count,state,newstate) 
			{
				newstate = fillABuffer(infile,totalwords,norm,my_n_words,count,*final_indices);
			}

			#pragma omp taskwait //This is so that only one buffer is written to disk at any one time
			#pragma omp flush(newstate)
			state = newstate;
			#pragma omp flush(state) //This might be uneccessary

		}

	//Once we reach here we still have the last buffer that needs to be written to disk.
		
		#pragma omp flush(buffercount)
		buffercount++;
		//Because fillABuffer() swapped the buffers even if it read in nothing, write out the other buffer here.
		writeBufferToDisk(buffercount,!current_page_group,*final_indices,!final_indices->get_current_buffer(),1);
	}
	}		
	//We've done all our reading from the input file.
	//We know that all merging is finished due to the implicit barrier at the end of the paralell section.
	
	int k = get_final_k();
	final_indices->copyToFinalPlace(k);

	char* output_location = (char*) malloc(strlen("ngrams_")+  4 + strlen(".metadata")+ 1); //MARKER5
	sprintf(output_location,"%u_grams.metadata",ngramsize);

	remove(output_location);

	FILE* metadata_file = fopen(output_location,"w");
	fprintf(metadata_file,"numwords\t%lld\n",totalwords);
  	gettimeofday(&end_time,NULL);
	fprintf(metadata_file,"time\t%d\n",(int)(end_time.tv_sec - start_time.tv_sec));

	delete final_indices;
	return totalwords;
}

