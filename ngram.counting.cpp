#include "ngram.counting.h"

/*Function declarations with documentation*/


/* This function first decrements offset_to_adjust once, and then continuously decrements it until (mmaped_file + offset_to_adjust) points
 * to the start of a region of continuous unicode space so that there are num_spaces regions of unicode space between
 * the original value of (mmaped_file + offset_to_adjust - 1) and the final value of (mmaped_file + offset_to_adjust)
 * including the region of spaces starting at (mmaped_file + offset_to_adjust).
 * mmaped_file needs to point to a region of memory of size file_size in utf-8 encoding.
 * If there do not exist num_spaces regions of space in the file, offset_to_adjust is set to 0.
 * num_spaces must not be 0.
 */
static void adjustBoundaryToSpace(const uint8_t* mmaped_file, off_t &offset_to_adjust,off_t file_size,unsigned int num_spaces);

/* This function gets the next character from an utf-8 region of memory pointed to by *ptr ending at eof.
 * *ptr is updated to point the the start of the next character.
 * The result is stored in *character.
 * On encountering something that is invalid utf-8, *ptr is incremented until a valid character sequence is found, or the
 * file ends.
 * Returns 1 on sucess and 0 if there is nothing more left to fetch in the file
 */
static int get_next_ucs4t_from_file( const uint8_t (**ptr),const uint8_t* eof,ucs4_t *character);

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
 * 		Currently Diacritics which precede are ignored. TODO: Research whether this is good or not.
 * 	Hyphen Characters are ignored unless they are within a word. 
 * 		For example: an input of --X-ray will result in the word x-ray being returned
 *	Punctuation characters are ignored.
 *	Any other character is also ignored.
 *
 * Any whitespace constitutes a word delimiter.
 * memmanagement_retval should point to an integer: *memmanagement_retval is set to -1 if the buffer should be switched as soon as possible.
 */
size_t getnextword(uint8_t * &s,const uint8_t (**f),const uint8_t *eof, uninorm_t norm,int &memmanagement_retval,Buffer* buf);


/* This function fills the Buffer specified by *buffer with words from the region of memory starting at mmaped_file and ending at *eof. It reads
 * from the memory at *f and updates *f so that it points to after the end of the last word read in.
 * totalwords is set to the number of words read in.
 * The words are normalized using norm.
 * The argument is_rightmost_buffer should be set to 1 if eof is the actual end of the mmaped file, i.e. there are no buffers
 * that start after this buffer.
 */
int fillABuffer(const uint8_t* mmaped_file,const uint8_t (**f),const uint8_t* eof, long long int &totalwords, uninorm_t norm, Buffer *buffer,int is_rightmost_buffer,unsigned int ngramsize);

/* This function generates indexes of n-grams of size ngramsize and the frequency that the n-grams occur at in the file infile_name
 * Setting wordsearch_index_depth to anything except for 1 or ngramsize is depricated
 * 	Setting it to 1 generates only one index, in which words are in the order as they are in the text.
 * 	Setting it to ngramsize generates ngramsize choose floor(ngramsize/2) indexes, allowing for any
 * 		combination of words (or lack of thereof) to be searched for.
 * 	cache_entire_file indicates whether we should tell the OS to cache the entire file or not.
 * 	The file is read concurrently from num_concurrent_buffers chunks of the file at a time.
 */
long long int count_ngrams(unsigned int ngramsize,const char* infile_name ,const char* outdir,unsigned int wordsearch_index_depth,unsigned int num_concurrent_buffers,bool cache_entire_file);


/*Function definitions*/



static void adjustBoundaryToSpace(const uint8_t* mmaped_file, off_t &offset_to_adjust,off_t file_size,unsigned int num_spaces)
{
	//As this function is only run very rarely, it is relatively unoptimized.

	//Whether or not the last character read was a whitespace character.
	int last_was_whitespace = 0;
	//How many characters were read previously.
	int read = 0;
	while(1)
	{
		ucs4_t character;
		if(!num_spaces && !last_was_whitespace)
		{
			//Since we're pointing to the first character before a region of whitespace,
			//we set mmaped_file + offset_to_adjust to point by the last character read forwards.
			if(read > 0)
			{
				offset_to_adjust += read;
			}
			else
			{
				offset_to_adjust++;
			}
			break;
		}else if(offset_to_adjust)
		{
			offset_to_adjust--;
		}else
		{
			break;
		}

		read = u8_mbtouc(&character, mmaped_file + offset_to_adjust,file_size - offset_to_adjust);

		if( (read > 0) && uc_is_property_white_space(character))
		{
			if(!last_was_whitespace)
			{
				num_spaces--;
			}
			last_was_whitespace = 1;
		}else
		{
			last_was_whitespace = 0;
		}
	}
	return;
}

static int get_next_ucs4t_from_file( const uint8_t (**ptr),const uint8_t* eof,ucs4_t *character)
{
	int read = 0;
	while(eof > *ptr)
	{
		read = u8_mbtouc(character,*ptr,eof-*ptr);
		if(read > 0)
			break;
		else
			(*ptr)++;
	}

	if(eof <= *ptr)
	{
		return 0;
	}else
	{
		(*ptr) += read;
	}

	return 1;
}

size_t getnextword(uint8_t * &s,const uint8_t (**f),const uint8_t *eof, uninorm_t norm,int &memmanagement_retval,Buffer* buf)
{

    size_t wordlength = 0; 
    uint8_t word[MAX_WORD_SIZE+1]; 

    ucs4_t character;
    while(get_next_ucs4t_from_file(f,eof,&character))
    { 
	if(uc_is_property_white_space(character))
	{
		if(!wordlength) //We ignore preceding whitespace.
			continue;
		else //We have reached the end of the word
			break;
	}else if(wordlength <= MAX_WORD_SIZE) 
	{
		if(uc_is_property_alphabetic(character))
		{
			character = uc_tolower(character);

		}else if(!(wordlength && 
				(uc_is_property_hyphen(character) || 
					 (uc_is_property_diacritic(character) && !uc_is_general_category(character, UC_MODIFIER_SYMBOL)))))
		{
			continue;
		}

		int read = u8_uctomb(word + wordlength,character,MAX_WORD_SIZE + 1 - wordlength);
		if(read < 0)
		{
			wordlength = MAX_WORD_SIZE + 1;
		}else
		{
			wordlength += read;
		}

	}
    }	

    if(wordlength > MAX_WORD_SIZE)
    {
        return MAX_WORD_SIZE + 1;
    }

    if(wordlength)
    {
	word[wordlength] = '\0';

	//---END.OF.DOCUMENT--- becomes END.OF.DOCUMENT--- because preceding hyphens are ignored.
    	if(!strcmp((const char*) word,"END.OF.DOCUMENT---")) 
    	{
		return MAX_WORD_SIZE + 1;
    	}


	//At this point we have an unnormalized lowercase unicode string.
	const size_t bytes_to_allocate = sizeof(*s) * (MAX_WORD_SIZE + 1);
	s = buf->allocate_for_string(bytes_to_allocate,memmanagement_retval);
	size_t retval = bytes_to_allocate;

	u8_normalize(norm,word, wordlength, s, &retval);

	if((retval > MAX_WORD_SIZE)) //This is possible because we allow the normalizer to write up to MAX_WORD_SIZE + 1 characters
	{
		buf->rewind_string_allocation(bytes_to_allocate);
		return MAX_WORD_SIZE + 1;

	}

	if((bytes_to_allocate- retval) > 0)
		buf->rewind_string_allocation(bytes_to_allocate - retval - sizeof(*s)); //The last sizeof(*s) is for the NUL at the end of the string.

	s[retval] = (uint8_t) '\0';

	return retval;
   }else
   {
	return 0;
  }
}



//Fills a buffer with words
int fillABuffer(const uint8_t* mmaped_file,const uint8_t (**f),const uint8_t* eof, long long int &totalwords, uninorm_t norm, Buffer *buffer,int is_rightmost_buffer,unsigned int ngramsize)
{
	int state = 1;
	uint8_t* word;

	if((ngramsize > 1) && (*f != mmaped_file))
	{
		//We read in what we need from the previous part of the file.
		off_t tmp_offset = *f - mmaped_file;

		//The following tells the buffer "there are 2*(ngramsize - 1) words in the buffer at the start that we do not have yet,
		//and because they'll be read in in reverse order, move your buffer pointer to 2*(ngramsize - 1)
		buffer->advance_to(2*(ngramsize - 1));
	
		//Keep track of how many words at the start of the buffer have been read in.
		//Note that we ignore any words read in here, because they were counted and read in by the previous buffer already.
		unsigned int counter = 0;

		int first = 1;
		int first_was_null = 0;

		while(counter < (2*(ngramsize - 1)))
		{
			off_t prev_offset = tmp_offset;
			adjustBoundaryToSpace(mmaped_file,tmp_offset,eof-mmaped_file, 1);
			if(!tmp_offset)
			{
				//We have reached the start of the file, meaning there aren't 2(ngramsize - 1) words in it.
				fprintf(stderr,"The chunks for reading in being used are too small. Maybe there are too many buffers for the file.\n");
				exit(-1);
			}

	
			const uint8_t* tmp_filePtr = mmaped_file + tmp_offset;
	
			size_t wordlength = getnextword(word,&tmp_filePtr, mmaped_file + prev_offset, norm,state, buffer);
			if(state != 1)
			{
				fprintf(stderr,"This should never happen and indicates that somewhere there is  bug (Or that you are using corpora with very long words at the start and very small buffers)\n");
				exit(-1);
			}
	
			if(!wordlength)//This means there was no word there, meaning we need to go back further to get to a word.
			{
				continue;
			}else if(wordlength > MAX_WORD_SIZE)
			{
				if(first)
					first_was_null = 1;
				buffer->add_null_word_at_start();
			}else
			{
				buffer->add_word_at_start(word);
				counter++;
			}
			first = 0;
		}
	
		buffer->add_null_word_at_start();
		buffer->advance_to(2*(ngramsize - 1));

		//We make sure things join at the seams.
		if(!first_was_null)
			buffer->set_current_word_as_last_word();
		else
			buffer->add_null_word();

		//How many words should not be starting words at the start of the buffer
		//(because they were starting words for the previous buffer)
		buffer->set_top_pointer(ngramsize - 1);
	}else
	{
		buffer->add_null_word();
		buffer->set_top_pointer(0);
	}


	while(state == 1)
	{
		size_t wordlength = getnextword(word,f,eof,norm,state,buffer);
		
		if(!wordlength) //We've reached the end of the file.
		{
			state =  0;
			break;
		}else if(wordlength > MAX_WORD_SIZE) //The word should not be counted as a word.
		{
			buffer->add_null_word();
		}else
		{
			buffer->add_word(word,state);
			totalwords++;

			if((totalwords % 1000000 == 0))
			{
				fprintf(stdout,".");
				fflush(stdout);
			}

		}
	}
	buffer->add_null_word();

	if(is_rightmost_buffer && (state == 0)) //There is nothing to be read in after this buffer.
		buffer->set_bottom_pointer(0);
	else //The last ngramsize - 1 words should not be starting words, because they will be starting words in the next buffer.
		buffer->set_bottom_pointer(ngramsize-1);

	return state;
}



long long int count_ngrams(unsigned int ngramsize,const char* infile_name ,const char* outdir,unsigned int wordsearch_index_depth,unsigned int num_concurrent_buffers,bool cache_entire_file)
{
    //Get the current time, for timing how long the function took.
    	struct timeval start_time,end_time;
  	gettimeofday(&start_time,NULL);
    //Initialize the local to read in UTF-8
	setlocale(LC_CTYPE,"en_US.UTF-8");

   //Open the input file, and get its size
	int fd = open(infile_name,O_RDONLY);
	off_t filesize = lseek(fd, 0,SEEK_END);

   //Initialize the merger
	size_t max_ngram_string_length = (ngramsize * (MAX_WORD_SIZE + 1)) + 1;
	init_merger(max_ngram_string_length);

   //Make the output directory(if it doesn't exist, and move to it)
	mkdir(outdir,S_IRUSR | S_IWUSR | S_IXUSR);
	chdir(outdir);

   //Create a new IndexCollection object representing all of the indexes we are making
	IndexCollection *indexes = new IndexCollection(ngramsize,wordsearch_index_depth);


   //We use this to normalize the unicode text. See http://unicode.org/reports/tr15/ for details on normalization forms.
	uninorm_t norm = UNINORM_NFKD; 


   //We map the file to a region of memory, telling the kernel to cache it where applicable.
	uint8_t* infile = (uint8_t*)mmap(NULL,filesize,PROT_READ, MAP_PRIVATE,fd,0);
	if((void*)infile == (void*) -1)
	{
		close(fd);
		fprintf(stderr,"Unable to mmap file %s\n",infile_name);
		exit(-1);
	}
	if(cache_entire_file)
	{
		if(madvise(infile,filesize,MADV_WILLNEED))
		{
			fprintf(stderr, "Unable to madvise with MADV_WILLNEED for file\n");
			exit(-1);
		}
	}

   //We split the file into num_concurrent_buffers (almost) equally sized chunks.
	vector<off_t> boundaries;
	boundaries.reserve(num_concurrent_buffers + 1);
	for(unsigned int i = 0; i<= num_concurrent_buffers; i++)
	{
		off_t current_boundary = (filesize * i) / num_concurrent_buffers;
		if((i != num_concurrent_buffers) && (i != 0))
		{
			adjustBoundaryToSpace(infile,current_boundary,filesize,1);
		}
		boundaries.push_back(current_boundary);
	}

   //We tell our scheduler about all of the the ranges for the different chunks.
	for(size_t i = 1; i <= num_concurrent_buffers; i++)
	{
		indexes->add_range_to_schedule(boundaries[i-1],boundaries[i]);
	}

   //We allocate the necessary memory for the buffers in advance, so that we don't run out of memory at some random point in time.
	vector<void*> internal_buffers;
	internal_buffers.reserve(num_concurrent_buffers);

	size_t buffer_size = MEMORY_TO_USE_FOR_BUFFERS/num_concurrent_buffers;
	//We need to store at least 2*ngramsize - 1 words per buffer, so checking for 2*ngramsize buffers is sufficient.
	if(buffer_size < (2*ngramsize*(MAX_WORD_SIZE + 1 + sizeof(word))))
	{
		fprintf(stderr, "Buffers are too small\n");
		exit(-1);
	}

	for(size_t i = 0; i<num_concurrent_buffers;i++)
	{
		void* cur = malloc(buffer_size);
		if(!cur)
		{
			fprintf(stderr,"Unable to allocate memory for buffers\n");
			exit(-1);
		}
		internal_buffers.push_back(cur);
	}


	long long int totalwords = 0;
   //Now we actually read in the file.
	#pragma omp parallel for
	for(size_t i = 0; i < num_concurrent_buffers; i++)
	{
		void* this_internal_buffer = internal_buffers[i];
		//We keep separate wordcounts per thread to avoid synchronisation slowdowns.
		long long int local_totalwords = 0;

		//Every thread loops until there is nothing left in the scheduler
		while(1)
		{
			pair<schedule_entry, int> current_schedule_entry = indexes->get_next_schedule_entry();
			if(!current_schedule_entry.second)
				break;


			off_t start = current_schedule_entry.first.start;
			off_t end = current_schedule_entry.first.end;

			//If we're not caching the whole thing, at least tell the kernel we'll be wanting the relevant region soon.
			if(!cache_entire_file)
			{
				//madvise needs page alignment..
				off_t page_start = (start/sysconf(_SC_PAGESIZE)) * sysconf(_SC_PAGESIZE);
				if(madvise(infile + page_start, end - page_start, MADV_SEQUENTIAL))
				{
					fprintf(stderr, "madvise failed\n");
					exit(-1);
				}
			}

			unsigned int local_buffercount = current_schedule_entry.first.buffercount;
			//To make swapping words in the sorting step easier, we have a special "null_word" instead of simply checking for NULL.
			//We define that here.
			//TODO: Check whether this actually makes the program more efficient or not.
			word local_null_word;
			Buffer* new_buffer = new Buffer(this_internal_buffer,buffer_size,(MAX_WORD_SIZE + 1) + sizeof(word),&local_null_word);

			int memmngt_retval = 1;
			const uint8_t* filePtr =  infile + start ;
			memmngt_retval = fillABuffer(infile,&filePtr,infile + end,local_totalwords,norm,new_buffer,end == filesize?1:0,ngramsize);

			//If it didn't all fit in this buffer, tell the scheduler about what's left.
			if(memmngt_retval == -1)
			{
				off_t new_start = filePtr - infile;
				if(new_start != end)
					indexes->add_range_to_schedule(new_start,end);
			}

			//We tell the scheduler that we're finished with this chunk. Note that it is important that we do so 
			//only *after* having told it about what's (potentially) left of the chunk
			indexes->mark_the_fact_that_a_range_has_been_read_in();

			//At this point, after having read in all of the words, we write them to disk.
			indexes->writeBufferToDisk(local_buffercount,0,new_buffer,&local_null_word);

			//Cleanup
			delete new_buffer;
		}
		#pragma omp atomic
		totalwords += local_totalwords;

		free(this_internal_buffer);
	}

   //Here we do various postprocessing operations, such as moving the files to their final positions, etc...
   //We also cleanup everything and free memory
   
	fprintf(stdout,"\n");
	fflush(stdout);
   
   //Throughout the code, k stands for the "merge depth".
	int k = get_final_k();
	indexes->copyToFinalPlace(k);

   //We output relevant metadata to a metadata file.
	char* output_location = (char*) malloc(strlen("ngrams_")+  4 + strlen(".metadata")+ 1); //MARKER5
	sprintf(output_location,"%u_grams.metadata",ngramsize);
	remove(output_location);
	FILE* metadata_file = fopen(output_location,"w");

	fprintf(metadata_file,"Filename:\t%s\n",infile_name);
	fprintf(metadata_file,"Numwords:\t%lld\n",totalwords);
  	gettimeofday(&end_time,NULL);
	fprintf(metadata_file,"Time:\t%d\n",(int)(end_time.tv_sec - start_time.tv_sec));
	indexes->writeMetadata(metadata_file);
	fclose(metadata_file);


	metadata_file = fopen(output_location,"r");
	free(output_location);

    //We write the contents of the metadata file to standard output.
	int c;
	while((c = fgetc(metadata_file)) != EOF)
	{
		fputc(c,stdout);
	};

   //Cleanup:
	delete indexes;
	munmap(infile,filesize);

	return totalwords;
}
