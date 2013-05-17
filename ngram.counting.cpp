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
static void adjustBoundaryToSpace(const uint8_t* mmaped_file, off_t &offset_to_adjust,off_t file_size,unsigned int num_spaces,bool is_pos);

/* This function gets the next character from an utf-8 region of memory pointed to by *ptr ending at eof.
 * *ptr is updated to point the the start of the next character.
 * The result is stored in *character.
 * On encountering something that is invalid utf-8, *ptr is incremented until a valid character sequence is found, or the
 * file ends.
 * Returns 1 on sucess and 0 if there is nothing more left to fetch in the file
 */
static int get_next_ucs4t_from_file( const uint8_t (**ptr),const uint8_t* eof,ucs4_t *character);

/* Adds the next string from *f to the relevant buffers. Note that it does *NOT* add a null_word on invalid words.
 * The string is normalized using norm
 *
 * Returns 2 if there was a subsequent word in f and it was valid.
 * Returns 1 if there was a subsequent word in f, but it was not valid (i.e. too long, poor pos formatting or ---END.OF.DOCUMENT---).
 * Returns 0 if there was no subsequent word in f. 
 *
 * The following transformations of the input string(s) from the file also take place:
 * 	Alphabetic characters from f are converted to their lowercase equivalents. (Note that this is done on a per-character basis)
 * 	Diacritics are left unmodified unless through normalization, or if they in the Unicode Sk category, in which case they are ignored.
 * 		Currently Diacritics which precede are ignored. TODO: Research whether this is good or not.
 * 	Hyphen Characters are ignored unless they are within a word. 
 * 		For example: an input of --X-ray will result in the word x-ray being returned
 *	Punctuation characters are ignored.
 *	Any other character is also ignored.
 *
 * Any whitespace constitutes a word delimiter.
 * If pos_buf is set, the word is ignored if the third delimiter is not a newline.
 * The word is added to the start of the buffer if add_word_at_start is set to true. If this is a case, remember to call Buffer::advance_to first.
 */
int getnextwordandaddit(
		const uint8_t (**f),
		const uint8_t *eof,
	       	uninorm_t norm,
		Buffer* buf,
	       	Buffer* pos_buf = NULL,
		bool add_word_at_start = false
		);


/* This function fills the Buffer specified by *buffer with words from the region of memory starting at mmaped_file and ending at *eof. It reads
 * from the memory at *f and updates *f so that it points to after the end of the last word read in.
 * totalwords is set to the number of words read in.
 * The words are normalized using norm.
 * The argument is_rightmost_buffer should be set to 1 if eof is the actual end of the mmaped file, i.e. there are no buffers
 * that start after this buffer.
 */
void fillABuffer(
		const uint8_t* mmaped_file,
		const uint8_t (**f),
		const uint8_t* eof,
	       	long long int &totalwords,
	       	uninorm_t norm,
	       	Buffer *buffer,
		int is_rightmost_buffer,
		unsigned int ngramsize,
		Buffer *pos_buffer = NULL
		);

/* This function generates indexes of n-grams of size ngramsize and the frequency that the n-grams occur at in the file infile_name
 * Setting wordsearch_index_depth to anything except for 1 or ngramsize is depricated
 * 	Setting it to 1 generates only one index, in which words are in the order as they are in the text.
 * 	Setting it to ngramsize generates ngramsize choose floor(ngramsize/2) indexes, allowing for any
 * 		combination of words (or lack of thereof) to be searched for.
 * 	cache_entire_file indicates whether we should tell the OS to cache the entire file or not.
 * 	The file is read concurrently from num_concurrent_buffers chunks of the file at a time.
 */
long long int count_ngrams(
		unsigned int ngramsize,
		const char* infile_name,
		const char* outdir,
		unsigned int wordsearch_index_depth,
		unsigned int num_concurrent_buffers,
		bool cache_entire_file,
		bool is_pos
		);


/*Function definitions*/

static void adjustBoundaryToSpace(const uint8_t* mmaped_file, off_t &offset_to_adjust,off_t file_size,unsigned int num_spaces,bool is_pos)
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
				if(!is_pos)
				{
					num_spaces--;
					last_was_whitespace = 1;
				}else
				{
					if(character == '\n')
					{
						num_spaces--;
						last_was_whitespace = 1;
					}else
					{
						last_was_whitespace = 0;
					}
				}
			}
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

int getnextwordandaddit(
		const uint8_t (**f),
		const uint8_t *eof, 
		uninorm_t norm,
		Buffer* ngram_buf,
	       	Buffer* pos_buf/* (= NULL)*/,
		bool add_at_start /*(= true)*/
		)
{

    uint8_t *s;
    uint8_t *inflexion_s =NULL;
    uint8_t *classification_s =NULL;
    uint8_t *lemma_s =NULL;

    uint8_t inflexion[MAX_WORD_SIZE+1];
    size_t inflexion_length = 0; 

    uint8_t classification[MAX_CLASSIFICATION_SIZE + 1];
    size_t classification_length = 0;

    uint8_t lemma[MAX_LEMMA_SIZE + 1];
    size_t lemma_length = 0;

    uint8_t* current_word_part = inflexion;
    size_t *current_word_length = &inflexion_length;
    size_t current_word_capacity = MAX_WORD_SIZE + 1;

    void* normalization_result;

    ucs4_t character;
    while(get_next_ucs4t_from_file(f,eof,&character))
    { 
	if(uc_is_property_white_space(character))
	{

		//TODO: find out whether comparing ucs4_t to a char is portable.
		if(pos_buf && (character == '\n'))
		{
			if(current_word_part == lemma)
			{
				break;
			}else if(inflexion_length > MAX_WORD_SIZE)
			{
				return 2;
			}else 
			{
				current_word_part = inflexion;
				current_word_length = &inflexion_length;
				current_word_capacity = MAX_WORD_SIZE + 1;

				inflexion_length = 0;
				classification_length = 0;
				lemma_length = 0;
				continue;
			}
		}

		if(!*current_word_length) //We ignore preceding whitespace.
			continue;

		else //We have reached the end of the word
		{
			if(!pos_buf)
				break;
			else
			{
				if(current_word_part == inflexion)
				{
					current_word_part = classification;
					current_word_length = &classification_length;
					current_word_capacity = MAX_CLASSIFICATION_SIZE + 1;
				}else if(current_word_part == classification)
				{
					current_word_part = lemma;
					current_word_length = &lemma_length;
					current_word_capacity = MAX_LEMMA_SIZE + 1;
				}else //This means there was no newline after the lemma, as the test above would have caught that.
				{
					return 2;
				}
			}
				
		}
	}else if(*current_word_length < current_word_capacity) 
	{
		if(uc_is_property_alphabetic(character))
		{
			character = uc_tolower(character);

		}else if(pos_buf && !*current_word_length && (character == '<'))
		{

			*current_word_length = current_word_capacity;
			continue;
		}else if(!(*current_word_length && 
				(uc_is_property_hyphen(character) || 
					 (uc_is_property_diacritic(character) && !uc_is_general_category(character, UC_MODIFIER_SYMBOL)))))
		{
			continue;
		}

		int read = u8_uctomb(current_word_part + *current_word_length,character,current_word_capacity - *current_word_length);
		if(read < 0)
		{
			*current_word_length = current_word_capacity;
		}else
		{
			*current_word_length += read;
		}

	}
    }	

    if(inflexion_length > MAX_WORD_SIZE)
    {
	    return 2;
    }

    if(pos_buf)
    {
	    if(classification_length > MAX_CLASSIFICATION_SIZE)
	    {
		return 2;
	    }
	    if(lemma_length > MAX_LEMMA_SIZE)
	    {
		return 2;
	    }
	    if(current_word_part != lemma)
	    {
		    return 0;
	    }


	    if( !classification_length || !lemma_length)
	    {
		    return 0;
	    }
   }
    
   inflexion[inflexion_length] = '\0';
   if(pos_buf)
   {
	   classification[classification_length] = '\0';
	   lemma[lemma_length] = '\0';
   }


    if(!inflexion_length)
    {
	    return 0;
    }else
    {

	//---END.OF.DOCUMENT--- becomes END.OF.DOCUMENT--- because preceding hyphens are ignored.
    	if(!strcmp((const char*) inflexion,"END.OF.DOCUMENT---")) 
    	{
		return 2;
    	}


	//At this point we have an unnormalized lowercase unicode string.
	size_t bytes_to_allocate = MAX_WORD_SIZE + 1;
	if(pos_buf)
	{
		bytes_to_allocate += MAX_CLASSIFICATION_SIZE + 1;
		bytes_to_allocate += MAX_LEMMA_SIZE + 1;
	}
	bytes_to_allocate *= sizeof(*s);

	s = ngram_buf->allocate_for_string(bytes_to_allocate);
	if(!pos_buf)
	{
		size_t inflexion_s_length = bytes_to_allocate;
		normalization_result = u8_normalize(norm,inflexion, inflexion_length, s, &inflexion_s_length);
	
		if(!normalization_result || (inflexion_s_length > MAX_WORD_SIZE)) //This is possible because we allow the normalizer to write up to MAX_WORD_SIZE + 1 characters
		{
			ngram_buf->rewind_string_allocation(bytes_to_allocate);
			return 2;
		}

		s[inflexion_s_length] = (uint8_t) '\0';
		inflexion_s_length++;
		ngram_buf->rewind_string_allocation(bytes_to_allocate - inflexion_s_length);
	}else
	{
		size_t classification_s_length = MAX_CLASSIFICATION_SIZE + 1;
		normalization_result = u8_normalize(norm,classification,classification_length,s,&classification_s_length);
		if(!normalization_result || (classification_s_length > MAX_CLASSIFICATION_SIZE))
		{
			ngram_buf->rewind_string_allocation(bytes_to_allocate);
			return 2;
		}
		classification_s = pos_buf->allocate_for_string(classification_s_length + 1);
		strncpy((char*)classification_s, (const char*)s, classification_s_length);
		classification_s[classification_s_length] = '\0';
		s[classification_s_length] = '|';

		uint8_t *ptr = s + classification_s_length + 1;

		size_t lemma_s_length = MAX_LEMMA_SIZE + 1;
		normalization_result = u8_normalize(norm,lemma,lemma_length,ptr, &lemma_s_length);
		if(!normalization_result || (lemma_s_length > MAX_LEMMA_SIZE))
		{
			ngram_buf->rewind_string_allocation(bytes_to_allocate);
			pos_buf->rewind_string_allocation(lemma_s_length + 1);
			return 2;
		}
		lemma_s = pos_buf->allocate_for_string(lemma_s_length + 1);
		strncpy((char*)lemma_s,(const char*) ptr, lemma_s_length);
		lemma_s[lemma_s_length] = '\0';
		ptr += lemma_s_length;
		*ptr = '|';
		ptr++;

		size_t inflexion_s_length = MAX_WORD_SIZE + 1;
		normalization_result = u8_normalize(norm,inflexion,inflexion_length,ptr, &inflexion_s_length);
		if(!normalization_result || (inflexion_s_length > MAX_WORD_SIZE))
		{
			ngram_buf->rewind_string_allocation(bytes_to_allocate);
			pos_buf->rewind_string_allocation(inflexion_s_length + 1);
			pos_buf->rewind_string_allocation(inflexion_s_length + 1);
		}
		inflexion_s = pos_buf->allocate_for_string(inflexion_s_length + 1);
		strncpy((char*)inflexion_s,(const char*) ptr, inflexion_s_length);
		inflexion_s[inflexion_s_length] = '\0';
		ptr += inflexion_s_length;
		*ptr = '\0';
		ptr++;
		ngram_buf->rewind_string_allocation( bytes_to_allocate - (ptr - s));

	}

	if(!add_at_start)
	{
		ngram_buf->add_word(s);
	}else
	{
		ngram_buf->add_word_at_start(s);
	}

	if(pos_buf)
	{
		pos_buf->add_null_word();
		pos_buf->add_word(inflexion_s);
		pos_buf->add_word(classification_s);
		pos_buf->add_word(lemma_s);
		pos_buf->add_null_word();
	}
	return 1;
   }
}



void fillABuffer(const uint8_t* mmaped_file,const uint8_t (**f),const uint8_t* eof, long long int &totalwords, uninorm_t norm, Buffer *buffer,int is_rightmost_buffer,unsigned int ngramsize,Buffer *pos_buffer/*( = NULL)*/)
{

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
			adjustBoundaryToSpace(mmaped_file,tmp_offset,eof-mmaped_file, 1,pos_buffer != NULL);
			if(!tmp_offset)
			{
				//We have reached the start of the file, meaning there aren't 2(ngramsize - 1) words in it.
				fprintf(stderr,"The chunks for reading in being used are too small. Maybe there are too many buffers for the file.\n");
				exit(-1);
			}

	
			const uint8_t* tmp_filePtr = mmaped_file + tmp_offset;
	
			//We don't bother adding these words to the pos buffer, as they've already been added to it.
			int res = getnextwordandaddit(&tmp_filePtr, mmaped_file + prev_offset, norm, buffer,pos_buffer,true);
			if(buffer->is_full)
			{
				fprintf(stderr,"This should never happen and indicates that somewhere there is  bug (Or that you are using corpora with very long words at the start and very small buffers)\n");
				exit(-1);
			}
	
			if(res == 0)//This means there was no word there, meaning we need to go back further to get to a word.
			{
				continue;
			}else if(res == 2)
			{
				if(first)
					first_was_null = 1;
				buffer->add_null_word_at_start();
			}else
			{
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


	while(!buffer->is_full && !(pos_buffer && pos_buffer->is_full))
	{
		int res = getnextwordandaddit(f,eof,norm,buffer,pos_buffer);
		
		if(!res) //We've reached the end of the file.
		{
			break;
		}else if(res == 2) //The word should not be counted as a word.
		{
			buffer->add_null_word();
		}else
		{
			totalwords++;

			if((totalwords % 1000000 == 0))
			{
				fprintf(stdout,".");
				fflush(stdout);
			}

		}
	}
	buffer->add_null_word();
	if(pos_buffer)
	{
		pos_buffer->add_null_word();
		pos_buffer->set_bottom_pointer(0);
		pos_buffer->set_top_pointer(0);
	}

	if(is_rightmost_buffer && !buffer->is_full && !(pos_buffer && pos_buffer->is_full)) //There is nothing to be read in after this buffer.
		buffer->set_bottom_pointer(0);
	else //The last ngramsize - 1 words should not be starting words, because they will be starting words in the next buffer.
		buffer->set_bottom_pointer(ngramsize-1);

	return;
}



long long int count_ngrams(unsigned int ngramsize,const char* infile_name ,const char* outdir,unsigned int wordsearch_index_depth,unsigned int num_concurrent_buffers,bool cache_entire_file,bool is_pos)
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
	if(chdir(outdir))
	{
		fprintf(stderr, "Chdir(2) failed");
		exit(-1);
	}

   //Create a new IndexCollection object representing all of the indexes we are making
	IndexCollection *indexes = new IndexCollection(ngramsize,wordsearch_index_depth);
	IndexCollection *pos_indexes;
	if(is_pos)
	{
		pos_indexes = new IndexCollection(3,3,true);
	}

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
			adjustBoundaryToSpace(infile,current_boundary,filesize,1,is_pos);
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
	size_t pos_buffer_size;

	vector<void*> pos_internal_buffers;
	if(is_pos)
	{
		pos_internal_buffers.reserve(num_concurrent_buffers);
		//Roughly how much memory we expect to be taking.
		buffer_size = ((MEMORY_TO_USE_FOR_BUFFERS * ngramsize)/(ngramsize + 3))/num_concurrent_buffers;
		//TODO: Think about alignment issues when allocating buffer size.
		pos_buffer_size = MEMORY_TO_USE_FOR_BUFFERS/num_concurrent_buffers - buffer_size; 
	}

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
		if(is_pos)
		{
			void* pos_cur = malloc(pos_buffer_size);
			if(!pos_cur)
			{
				fprintf(stderr,"Unable to allocate memory for buffers\n");
				exit(-1);
			}
			pos_internal_buffers.push_back(pos_cur);
		}
	}


	long long int totalwords = 0;
   //Now we actually read in the file.
	#pragma omp parallel for
	for(size_t i = 0; i < num_concurrent_buffers; i++)
	{
		void* this_internal_buffer = internal_buffers[i];
		void* this_pos_internal_buffer = NULL;
		if(is_pos)
		{
			this_pos_internal_buffer = pos_internal_buffers[i];
		}
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
			Buffer* pos_new_buffer = NULL;

			if(is_pos)
			{
				pos_new_buffer = new Buffer(
						this_pos_internal_buffer,
						pos_buffer_size,
						MAX_WORD_SIZE + 1 + MAX_CLASSIFICATION_SIZE + 1 + MAX_LEMMA_SIZE + 1 + 3*sizeof(word),
						&local_null_word
						);
			}

			const uint8_t* filePtr =  infile + start ;
			fillABuffer(infile,&filePtr,infile + end,local_totalwords,norm,new_buffer,end == filesize?1:0,ngramsize,pos_new_buffer);

			//If it didn't all fit in this buffer, tell the scheduler about what's left.
			if(new_buffer->is_full || (is_pos && pos_new_buffer->is_full))
			{
				off_t new_start = filePtr - infile;
				if(new_start != end)
					indexes->add_range_to_schedule(new_start,end);
			}

			//We tell the scheduler that we're finished with this chunk. Note that it is important that we do so 
			//only *after* having told it about what's (potentially) left of the chunk
			bool this_was_last = indexes->mark_the_fact_that_a_range_has_been_read_in();
			if(this_was_last)
			{
				indexes->writeEmptyLastBuffer(local_buffercount+1);
				if(is_pos)
					pos_indexes->writeEmptyLastBuffer(local_buffercount+1);
			}

			//At this point, after having read in all of the words, we write them to disk.
			indexes->writeBufferToDisk(local_buffercount,0,new_buffer,&local_null_word);
			if(is_pos)
			{
				pos_indexes->writeBufferToDisk(local_buffercount,0,pos_new_buffer,&local_null_word);
			}

			//Cleanup
			delete new_buffer;
			if(is_pos)
			{
				delete pos_new_buffer;
			}
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
	if(is_pos)
	{
		pos_indexes->copyToFinalPlace(k);
	}

   //We output relevant metadata to a metadata file.
	char* output_location = (char*) malloc(strlen("ngrams_")+  4 + strlen(".metadata")+ 1); //MARKER5
	sprintf(output_location,"%u_grams.metadata",ngramsize);
	remove(output_location);
	FILE* metadata_file = fopen(output_location,"w");

	fprintf(metadata_file,"Filename:\t%s\n",infile_name);
	fprintf(metadata_file,"Numwords:\t%lld\n",totalwords);
  	gettimeofday(&end_time,NULL);
	fprintf(metadata_file,"Time:\t%d\n",(int)(end_time.tv_sec - start_time.tv_sec));
	fprintf(metadata_file,"MAX_WORD_SIZE:\t%d\n", MAX_WORD_SIZE);
	fprintf(metadata_file,"MAX_CLASSIFICATION_SIZE:\t%d\n", MAX_CLASSIFICATION_SIZE);
	fprintf(metadata_file,"MAX_LEMMA_SIZE:\t%d\n", MAX_LEMMA_SIZE);

	indexes->writeMetadata(metadata_file);
	if(is_pos)
	{
		pos_indexes->writeMetadata(metadata_file);
	}
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
	if(is_pos)
	{
		delete pos_indexes;
	}
	munmap(infile,filesize);

	return totalwords;
}
