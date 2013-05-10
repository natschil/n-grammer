#include "ngram.counting.h"

using namespace std;

/*Some global variables*/

//These two global variables are set the start of count_ngrams(), and are not excpected to change after that
int n_gram_size = 0;
int max_ngram_string_length = 0;

/*The rest of the code*/


//Returns 0 if there is no next character.
static int get_next_ucs4t_from_file( const char (**ptr),const char* eof,ucs4_t *character)
{
	int read = u8_mbtouc(character,(const uint8_t*)*ptr,eof - *ptr);
	while(eof > *ptr &&  read <= 0)
	{
		(*ptr)++;
		read = u8_mbtouc(character,(const uint8_t*) *ptr,eof - *ptr);
	}

	if(*ptr == eof)
		return 0;

	(*ptr) += read;
	return 1;
}

/*
 *This function looks at the region of UTF-8 memory pointed to by mmaped_file + offset_to_adjust
 *And sets offset_to_adjust backward by num_spaces regions of Unicode whitespace, or to 0, whichever comes first,
 * so that mmaped_file + offset_to_adjust points to the start of a region of whitespace, or to the start of the file.
 */
static void adjustBoundaryToSpace(const char* mmaped_file, off_t &offset_to_adjust,off_t file_size,int num_spaces)
{
	ucs4_t character;
	int read;
	int last_was_whitespace = 0;
	while(1)
	{
		if(!num_spaces && !last_was_whitespace)
		{
			offset_to_adjust++;
			break;
		}else
		{
			offset_to_adjust--;
		}
		if(!offset_to_adjust)
			break;
		read = u8_mbtouc(&character,(const uint8_t*)( mmaped_file + offset_to_adjust),file_size - offset_to_adjust);
		if( (read > 0) && uc_is_property_white_space(character))
		{
			if(num_spaces && !last_was_whitespace)
			{
				num_spaces--;
			}
			last_was_whitespace = 1;
		}else
			last_was_whitespace = 0;
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


size_t getnextword(uint8_t * &s,const char (**f),const char *eof, uninorm_t norm,int &memmanagement_retval,Buffer* buf)
{

    size_t wordlength = 0; 
    uint8_t word[MAX_WORD_SIZE+1]; 

    ucs4_t character;
    for(; get_next_ucs4t_from_file(f,eof,&character);)
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
	s = buf->allocate_for_string(bytes_to_allocate,memmanagement_retval);
	size_t retval = bytes_to_allocate;

	u8_normalize(norm,word, wordlength, s, &retval);

	if((retval > MAX_WORD_SIZE)) //This is possible because we allow the normalizer to write up to MAX_WORD_SIZE + 1 characters
	{
		buf->rewind_string_allocation(bytes_to_allocate);
		retval = MAX_WORD_SIZE + 1;
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
int fillABuffer(const char* mmaped_file,const char (**f),const char* eof, long long int &totalwords, uninorm_t norm, Buffer *buffer,int is_rightmost_buffer)
{
	int state = 1;
	uint8_t* word;

	if((n_gram_size > 1) && (*f != mmaped_file))
	{
		//We read in what we need from the previous part of the file.
		off_t tmp_offset = *f - mmaped_file;
		buffer->advance_to(2*(n_gram_size - 1));
	
		int counter = 0;
		int first = 1;
		int first_was_null = 0;
		while(counter < (2*(n_gram_size - 1)))
		{
			off_t prev_offset = tmp_offset;
			adjustBoundaryToSpace(mmaped_file,tmp_offset,eof-mmaped_file, 1);
			if(!tmp_offset)
			{
				fprintf(stderr,"The buffers being used are too small\n");
				exit(-1);
			}

	
			const char* tmp_filePtr = mmaped_file + tmp_offset;
	
			size_t wordlength = getnextword(word,&tmp_filePtr, mmaped_file + prev_offset, norm,state, buffer);
			if(state != 1)
			{
				fprintf(stderr,"This should never happen and indicates that somewhere there is  bug\n");
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
		buffer->advance_to(2*(n_gram_size - 1));

		if(!first_was_null)
			buffer->set_current_word_as_last_word();
		else
			buffer->add_null_word();

		buffer->set_top_pointer(n_gram_size - 1);
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

	if(is_rightmost_buffer && (state == 0))
		buffer->set_bottom_pointer(0);
	else
		buffer->set_bottom_pointer(n_gram_size-1);

	return state;
}



long long int count_ngrams(unsigned int ngramsize,const char* infile_name ,const char* outdir,unsigned int wordsearch_index_depth,unsigned int num_concurrent_buffers,bool cache_entire_file)
{
    //Initialization:
	setlocale(LC_CTYPE,"");
    	struct stat st;
	if(stat(infile_name, &st))
	{
		fprintf(stdout,"Failed to stat %s\n", infile_name);
		exit(-1);
	}
	off_t filesize = st.st_size;
	int fd = open(infile_name,O_RDONLY);

    	struct timeval start_time,end_time;
  	gettimeofday(&start_time,NULL);

	n_gram_size = ngramsize;
	max_ngram_string_length = (ngramsize * (MAX_WORD_SIZE + 1)) + 1;
	#pragma omp flush(n_gram_size,max_ngram_string_length) //This is probably uneccessary

	init_merger(max_ngram_string_length);

	mkdir(outdir,S_IRUSR | S_IWUSR | S_IXUSR);
	chdir(outdir);

	long long int totalwords = 0;
	IndexCollection *indexes = new IndexCollection(ngramsize,wordsearch_index_depth);

	//We use this to normalize the unicode text. See http://unicode.org/reports/tr15/ for details on normalization forms.
	uninorm_t norm = UNINORM_NFKD; 


	//We now mmap the file...
	void* infile = mmap(NULL,filesize,PROT_READ, MAP_PRIVATE,fd,0);
	if(infile == (void*) -1)
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

	vector<off_t> boundaries;
	boundaries.reserve(num_concurrent_buffers + 1);
	for(unsigned int i = 0; i<= num_concurrent_buffers; i++)
	{
		boundaries.push_back((filesize * i)/num_concurrent_buffers);
	}

	for(unsigned int i = 1; i < num_concurrent_buffers; i++)
	{
		adjustBoundaryToSpace((const char*)infile,boundaries[i],filesize,1);
	}

	for(size_t i = 1; i <= num_concurrent_buffers; i++)
	{
		indexes->add_range_to_schedule(boundaries[i-1],boundaries[i]);
	}

	vector<void*> internal_buffers;
	internal_buffers.reserve(num_concurrent_buffers);


	//We actually need to store at least 2*ngramsize - 1 wordds.
	if((MEMORY_TO_USE_FOR_BUFFERS/num_concurrent_buffers) < (2*ngramsize*(MAX_WORD_SIZE + 1 + sizeof(word))))
	{
		fprintf(stderr, "Buffers are too small\n");
		munmap(infile,filesize);
		exit(-1);
	}
	for(size_t i = 0; i<num_concurrent_buffers;i++)
	{
		void* cur = malloc(MEMORY_TO_USE_FOR_BUFFERS/num_concurrent_buffers);
		if(!cur)
		{
			fprintf(stderr,"Unable to allocate memory for buffers\n");
			munmap(infile,filesize);
			exit(-1);
		}
		internal_buffers.push_back(cur);
	}

	#pragma omp parallel for
	for(size_t i = 0; i < num_concurrent_buffers; i++)
	{
		void* this_internal_buffer = internal_buffers[i];
		long long int local_totalwords = 0;
		while(1)
		{
			pair<schedule_entry, int> current_schedule_entry = indexes->get_next_schedule_entry();
			if(!current_schedule_entry.second)
				break;

			off_t start = current_schedule_entry.first.start;
			off_t end = current_schedule_entry.first.end;

			off_t page_start = (start/sysconf(_SC_PAGESIZE)) * sysconf(_SC_PAGESIZE);
			if(!cache_entire_file)
			{
				if(madvise(infile + page_start, end - page_start, MADV_SEQUENTIAL))
				{
					fprintf(stderr, "madvise failed\n");
					exit(-1);
				}
			}


			unsigned int local_buffercount = current_schedule_entry.first.buffercount;

			word local_null_word;

			Buffer* new_buffer = new Buffer(this_internal_buffer,MEMORY_TO_USE_FOR_BUFFERS/num_concurrent_buffers,(MAX_WORD_SIZE + 1) + sizeof(word),&local_null_word);
			int memmngt_retval = 1;
			const char* filePtr = (const char*)( infile + start) ;
			memmngt_retval = fillABuffer((const char*) infile,&filePtr,(const char*) (infile + end),local_totalwords,norm,new_buffer, end == filesize? 1:0 );
			if(memmngt_retval == -1)
			{
				off_t new_start = (const char*)filePtr -(const char*) infile;
				if(new_start != end)
					indexes->add_range_to_schedule(new_start,end);
			}
			indexes->mark_the_fact_that_a_range_has_been_read_in();
			indexes->writeBufferToDisk(local_buffercount,0,new_buffer,&local_null_word);
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
   
	int k = get_final_k();
	indexes->copyToFinalPlace(k);

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
