#include "ngram.counting.h"

using namespace std;

/*Some global variables*/

//These two global variables are set the start of count_ngrams(), and are not excpected to change after that
int n_gram_size = 0;
int max_ngram_string_length = 0;

/*The rest of the code*/


//Returns 0 if there is no next character.
 int get_next_ucs4t_from_file(FILE* f,ucs4_t *character)
{
	static uint8_t buf[4];
	static int numwords = 0;
	for(; numwords<4;numwords++)
	{
		int c = fgetc(f);
		if(c == EOF)
			break;
		buf[numwords] = c;
	}
	int read = u8_mbtouc(character, buf, numwords);
	while(numwords && (read <= 0))
	{
		//We shift right;
		for(int i = 1; i < numwords; i++)
		{
			buf[i -1] = buf[i];
		}
		int c = fgetc(f);
		if(c == EOF)
			numwords--;
		else
			buf[numwords - 1] = c;

		read = u8_mbtouc(character,buf,numwords);
	}
	if(numwords)
	{
		for(int i = read; i < numwords; i++)
		{
			buf[i - read] = buf[i];
		}	
		numwords -= read;
		return 1;
	}else
	{
		return 0;
	}

}

/*Sets s to be a pointer to a (uint8_t, NUL terminated) string of the next word from f.
 * s is allocated using index_collection.allocate_for_string, and normalized using norm.
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


size_t getnextword(uint8_t* &s,FILE* f,uninorm_t norm,int &memmanagement_retval,IndexCollection &index_collection)
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
	s = index_collection.allocate_for_string(bytes_to_allocate,memmanagement_retval);
	size_t retval = bytes_to_allocate;

	u8_normalize(norm,word, wordlength, s, &retval);

	if((retval > MAX_WORD_SIZE)) //This is possible because we allow the normalizer to write up to MAX_WORD_SIZE + 1 characters
	{
		index_collection.rewind_string_allocation(bytes_to_allocate);
		retval = MAX_WORD_SIZE + 1;
	}

	if((bytes_to_allocate- retval) > 0)
		index_collection.rewind_string_allocation(bytes_to_allocate - retval - sizeof(*s)); //The last sizeof(*s) is for the NUL at the end of the string.

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

int mark_next_word(FILE* f,long long int &totalwords,uninorm_t n,word_list &my_n_words,NGram *&ngram,IndexCollection &index_collection)
{
	int retval;
	uint8_t* word;

	retval = 1;
	size_t wordlength = getnextword(word,f,n,retval,index_collection);
		
	if(!wordlength) //We've reached the end of the file.
	{
		retval =  0;
		break;
	}else if(wordlength > MAX_WORD_SIZE) //The word should not be counted as a word.
	{
		index_collection.mark_null_word(retval);
	}else
	{
		index_collection.add_word(retval);
	}

	return retval;
}




/*Writes the dictionary associated with the buffer page_group to disk*/
static void writeBufferToDisk(int buffercount,size_t page_group,IndexCollection &index_collection,size_t buffer_num,int isfinalbuffer)
{
		setpagelock(page_group);
		fprintf(stderr,"Writing Buffer %d to disk\n",buffercount);
		fflush(stderr);
		index_collection.writeBufferToDisk(buffer_num, buffercount,isfinalbuffer);
		#pragma omp flush
		unsetpagelock(page_group);
}


//Fills a buffer with words and n-grams.
int fillABuffer(FILE* f, long long int &totalwords, uninorm_t norm, word_list &my_n_words,long long int &count,IndexCollection &indices)
{

	//We get the lock for the current page group.
	//If anything else still has the lock for it, this function will wait for that to finish.
	setpagelock(current_page_group);

	//Get whatever words are still in the old buffer and copy them to the new buffer
	//The old buffer has not been modified because nothing writes to the buffers except fillABuffer
	my_n_words.migrate_to_new_buffer();

	int state = 1;
	NGram *currentngram;
	while(state == 1)
	{
		state = mark_next_word(f, totalwords, norm, my_n_words,currentngram,indices);

		count++;
		if((count % 1000000 == 0))
		{
			fprintf(stderr,"%lld\n",(long long int) count);
		}
	}
	//BACKHERE
	//When the buffer almost full, we switch buffers:
	switch_permanent_malloc_buffers();	
	indices.swapBuffers();

	unsetpagelock(!current_page_group);

	return state;
}

long long int count_ngrams(unsigned int ngramsize,const char* input_file ,const char* outdir,unsigned int wordsearch_index_depth)
{
    //Initialization:
    	FILE* infile = input_file ? fopen(input_file,"r") : stdin;
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

	IndexCollection *final_indices = new IndexCollection(ngramsize,wordsearch_index_depth);

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
				size_t previous_buffer = !final_indices->getCurrentBuffer();
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
		writeBufferToDisk(buffercount,!current_page_group,*final_indices,!final_indices->getCurrentBuffer(),1);
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
	fprintf(metadata_file,"Filename:\t%s\n",input_file);
	fprintf(metadata_file,"Numwords:\t%lld\n",totalwords);
  	gettimeofday(&end_time,NULL);
	fprintf(metadata_file,"Time:\t%d\n",(int)(end_time.tv_sec - start_time.tv_sec));
	final_indices->writeMetadata(metadata_file);
	fclose(metadata_file);
	metadata_file = fopen(output_location,"r");
	free(output_location);
	int c;
	while((c = fgetc(metadata_file)) != EOF)
	{
		fputc(c,stderr);
	};

	delete final_indices;
	return totalwords;
}
