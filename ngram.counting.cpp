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
	int read = u8_mbtouc(character,buf,eof - ptr);
	while(endtpr > ptr &&  read <= 0)
	{
		(*ptr)++;
		read = u8_mbtouc(character,buf,numwords,eof - ptr);
	}

	if(ptr == endptr)
		return 0;

	(*ptr) += read;
	return 1;
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


size_t getnextword(const char (**f), const char *eof, uninorm_t norm,int &memmanagement_retval,Buffer* buf)
{

    uint8_t* s;
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



//Fills a buffer with words and n-grams.
int fillABuffer(const char (**f),const char* eof, long long int &totalwords, uninorm_t norm, Buffer *buffer)
{
	int state = 1;
	while(state == 1)
	{

		size_t wordlength = getnextword(f,eof,norm,state,buffer);
		
		if(!wordlength) //We've reached the end of the file.
		{
			state =  0;
			break;
		}else if(wordlength > MAX_WORD_SIZE) //The word should not be counted as a word.
		{
			indices.add_null_word();
		}else
		{
			indices.add_word(word,state);
		}

		totalwords++;//TODO: Figure out what exactly should be counted as a word...

		if((totalwords % 1000000 == 0))
		{
			fprintf(stdout,"%lld\n",(long long int) totalwords/1000000);
		}
	}
	indices->add_null_word();
	return state;
}

long long int count_ngrams(unsigned int ngramsize,FILE* infile ,const char* outdir,unsigned int wordsearch_index_depth)
{
    //Initialization:
    	struct timeval start_time,end_time;
  	gettimeofday(&start_time,NULL);

	n_gram_size = ngramsize;
	max_ngram_string_length = (ngramsize * (MAX_WORD_SIZE + 1)) + 1;
	#pragma omp flush(n_gram_size,max_ngram_string_length) //This is probably uneccessary

	init_merger(max_ngram_string_length);
	setlocale(LC_CTYPE,"");

	mkdir(outdir,S_IRUSR | S_IWUSR | S_IXUSR);
	chdir(outdir);

	long long int totalwords = 0;
	IndexCollection *indexes = new IndexCollection(ngramsize,wordsearch_index_depth);

	//We use this to normalize the unicode text. See http://unicode.org/reports/tr15/ for details on normalization forms.
	uninorm_t norm = UNINORM_NFKD; 

	fseek(infile,0,SEEK_END);
	off_t filesize = ftell(infile);

	char* num_available_threads_c = getenv("OMP_NUM_THREADS");
	int num_available_threads = 1;
	if(num_available_threads_c)
	{
		num_avilable_threads = atoi(num_available_threads_c);
	}

	size_t buffer_size = (MEMORY_TO_USE_FOR_BUFFERS/THEORETICAL_MAX_BUFFER_SIZE) > num_available_threads ? 


	size_t num_concurrent_buffers = min(MEMORY_TO_USE_FOR_BUFFERS/THEORETICAL_MAX_BUFFER_SIZE,num_available_threads);

   //Here we do various postprocessing operations, such as moving the files to their final positions, etc...
   //We also cleanup everything and free memory etc...
   
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
		fputc(c,stdout);
	};

	delete final_indices;
	return totalwords;
}
