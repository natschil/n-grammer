#include "ngram.counting.h"

using namespace std;
using namespace icu;

//Some global variables
letterDict *lexicon = NULL;
letterDict *prev_lexicon = NULL;
int n_gram_size = 0;
int max_ngram_string_length = 0;
UChar* space;

//Some function declarations.
static void mark_ngram_occurance(myNGram new_ngram);
static void swapDicts();
static void* writeLetterDict(int buffercount,int number,letterDict* old_dict);
void writeLetterDictToFile(letterDict &sublexicon,UFILE* outfile);
struct word_list;

int getnextngram(UFILE* f,long long int &totalwords,const UNormalizer2* n,word_list &my_n_words,myNGram &str);

bool ngram_cmp::operator()(myNGram first, myNGram second)
{
	for(int i = 0; i<n_gram_size; i++)
	{
		int prevres;
		if(!(prevres = (u_strcmp(first.ngram[i],second.ngram[i]))))//Makes merging easier because we don't need to convert to UTF16 again..
		{
			continue;
		}
		else return prevres < 0;
	}
	return false;
}

myNGram::~myNGram(){return;};

struct word_list : public std::deque<myUString>
{
	word_list() : std::deque<myUString>()
	{
		this->size = 0;
		this->uchars_used = 0;
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
		this->uchars_used = 0;
		return;
	}

	void pop_front()
	{
		this->size--;
		myUString old_string = this->front();
		this->uchars_used -= (old_string.length + 1);
		this->std::deque<myUString>::pop_front();
		return;
	}

	void push_back(myUString newstring)
	{
		this->size++;
		this->uchars_used += (newstring.length + 1);
		this->std::deque<myUString>::push_back(newstring);
	}

	//This function copies everything in the word list to a new buffer.
	void migrate_to_new_buffer()
	{
		int memmgnt_retval = 1;
		UChar* in_new_buffer = (UChar*) permanently_malloc(this->uchars_used * sizeof(UChar),&memmgnt_retval);
		if(memmgnt_retval == -1)
		{
			fprintf(stderr,"Buffer size too small. Please reconfigure, recompile and rerun\n");
			exit(-1);
		}
		UChar* ptr = in_new_buffer;
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
		result.ngram = (UChar**) permanently_malloc(sizeof(*result.ngram) * n_gram_size,&memory_management_retval);
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
			result.ngram[j] = (*i).string;			
		}
		//We return the n-gram
		return result;
	}
	int size;
	private:
	int uchars_used;
};

#define MAX_WORD_SIZE 40
//Sets s to point to a normalized UChar* with the next word from f. s is allocated using permenently_malloc, and normalized using n.
//Returns the size of f iff there is a next word.
//Returns 0 if there is no subsequent word in f. The value of s is undefined if this is the case.
//Returns x so that x > MAX_WORD_SIZE if the contents of s, which are left undefined, should be ignored.
//Alphabetic characters from f are converted to their lowercase equivalents.
//Diacritics are left unmodified unless through normalization.
//Hyphen Characters are ignored unless they are within a word. Therefore an input of --X-ray will result in the word x-ray being returned
//Punctuation and Digit Characters are Ingored
//Any whitespace constitutes a word delimiter.
//Any other characters are ignored.


int getnextword(UChar* &s,UFILE* f,const UNormalizer2* n,int *memmanagement_retval)
{

    int wordlength = 0; 
    int first_is_hyphen = 0;
    UChar word[MAX_WORD_SIZE+1]; //words may not be longer than MAX_WORD_SIZE characters.
    //The size is MAX_WORD_SIZE + 1 so that it can hold a word of length MAX_WORD_SIZE+1 which is longer than MAX_WORD_SIZE characters and will hence trigger it to be ignored in 
    //The next code.
    UChar32 character;

    //See http://icu-project.org/apiref/icu4c/ustdio_8h.html#a48b9be06c611643848639fa4c22a16f4
    for(wordlength = 0;((character = u_fgetcx(f)) != U_EOF);)
    { 
	//We now check whether or not the character is a whitespace character:
	if(u_isUWhiteSpace(character))
	{
		if(!wordlength) //We ignnore preceding whitespace.
			continue;
		else //We have reached the end of the word
			break;
	}else if(wordlength <= MAX_WORD_SIZE) //After 41 characters we ignore all non-whitespace stuff.
	{
		if(u_isUAlphabetic(character))
		{
			UChar32 lowercase = u_tolower(character);

			word[wordlength] = lowercase;
			wordlength++;
		}else if(u_hasBinaryProperty(character, UCHAR_HYPHEN) || u_hasBinaryProperty(character, UCHAR_DIACRITIC ))
		{
			if(u_hasBinaryProperty(character,UCHAR_HYPHEN))
			{
				first_is_hyphen = 1;	
			}
			if(wordlength)
			{
				//We treat hyphens like we treat normal characters..
				word[wordlength] = character;
				wordlength++;
			}
		}else if(u_ispunct(character) || u_isdigit(character))
		{
			continue;
		}else if(!U16_IS_SINGLE(character))
		{
			//We could relatively easily split c into it's surrogate pair,
			//but it is probably better to simply ignore the character and print an error:
			fprintf(stderr, "Found non-UTF16 character %x\n", (int) character);
			continue;
		}else
		{
			//This happens on some characters such as $ or °.
		}
	}
    }	
    if(wordlength > MAX_WORD_SIZE)
    {
	    return MAX_WORD_SIZE + 1;
    }
    word[wordlength] = '\0';
    if(first_is_hyphen && (wordlength == strlen("END.OF.DOCUMENT---")))
    {
	size_t itr = 0;
	for(itr = 0; itr < strlen("END.OF.DOCUMENT---");itr++)
	{
		if(((char) word[itr]) != "END.OF.DOCUMENT"[itr])
		{
			break;
		}
	}
	if(itr == strlen("END.OF.DOCUMENT---"))
	{
		return MAX_WORD_SIZE + 1;
	}
    }

    if(wordlength)
    {
	//At this point we have a unicode string. However, normalization issues are still present
	s = (UChar*) permanently_malloc(sizeof(*s) * (MAX_WORD_SIZE + 1),memmanagement_retval);
	UErrorCode e = U_ZERO_ERROR;
	int retval =  unorm2_normalize(n,word,wordlength,s,MAX_WORD_SIZE,&e);
	if(!U_SUCCESS(e))
	{
		rewind_permanent_malloc(sizeof(*s) * (MAX_WORD_SIZE + 1));	
		retval = MAX_WORD_SIZE + 1;
	}
	s[retval] = (UChar) 0;
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
	UFILE* outfile = u_fopen(buf,"w","UTF-8",NULL);
	if(!outfile)
	{
		fprintf(stderr,"Unable to open file %s", buf);
		exit(-1);
	}

	writeLetterDictToFile(old_dict[i],outfile);
	u_fclose(outfile);
	//MARKER1 relies on the following step
	old_dict[i].clear();
	return NULL;
}


void writeLetterDictToFile(letterDict &D,UFILE* outfile)
{
	long long int freq;

	for (letterDict::iterator i = D.begin(); i != D.end(); i++) {
		UChar** n_gram = i->first.ngram;
		for(int j = 0; j< n_gram_size; j++)
		{
			u_fprintf(outfile,"%S", n_gram[j]);
			if(j != (n_gram_size -1))
			{
				u_fprintf(outfile," ");
			}
		}
		freq = i->second;
		u_fprintf(outfile,"\t%lld\n", (long long int) freq);
	}
}


static void mark_ngram_occurance(myNGram new_ngram)
{
	
	//We run a kind of bucket-sort which hopefully makes things slightly faster.
	size_t firstchar = (size_t) (unsigned char) new_ngram.ngram[0][0];
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

static void mergeNewBuffers(int buffercount)
{
	#pragma omp parallel for firstprivate(buffercount) shared(max_ngram_string_length)
	for(int i = 0; i< 256;i++)
	{
		int n = buffercount;
		int k = 0;
		while(n % 2)
		{
			char buf[256];
			char buf2[256];
			char output[256];
			char outdir[256];
			snprintf(buf, 256, "./%d_%d/%d.out",k,n,i);
			snprintf(buf2, 256,"./%d_%d/%d.out",k,n-1,i);
			n = (n - 1)/2;
			k++;
			snprintf(outdir, 256,"./%d_%d",k,n);
			mkdir(outdir,S_IRUSR | S_IWUSR | S_IXUSR);
			snprintf(output, 256,"./%d_%d/%d.out",k,n,i);
			FILE* firstfile = fopen(buf, "r");
			FILE* secondfile = fopen(buf2, "r");
			FILE* outputfile = fopen(output, "w");
			if(!firstfile)
			{
				fprintf(stderr, "Unable to open %s for reading", buf);
				exit(-1);
			}else if(!secondfile)
			{
				fprintf(stderr,"Unable to open %s for reading", buf2);
				exit(-1);
			}else if(!outputfile)
			{
				fprintf(stderr, "Unable to open %s for writing",output);
				exit(-1);
			}
			if(!i)
				fprintf(stderr,"Mergeing %s with %s to give %s\n", buf, buf2, output);
			int mergefile_out = merge_files(firstfile,secondfile,outputfile,max_ngram_string_length);
			fclose(firstfile);
			fclose(secondfile);
			fclose(outputfile);
			if(mergefile_out)
			{
				fprintf(stderr, "Failed to merge files %s and %s\n",buf,buf2);
				exit(-1);
			}
		}	
	}
}
int fillABuffer(UFILE* f, long long int &totalwords, const UNormalizer2* norm, word_list &my_n_words,long long int &count)
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

std::pair<int,int> doLastMerge(int n, int k,int buffercount)
{
	k = 0;
	#pragma omp parallel for firstprivate(n) firstprivate(k) lastprivate(n) lastprivate(k)
	for(int i = 0; i< 256;i++)
	{
		n = buffercount;
		k = 0;
		while(n)
		{
			int n2 = n -1;
			int k2 = k;
			while(n2 && (n2 % 2)) //Go the the next "peak" in the merge tree.
			{
				n2 = (n2 - 1)/2;
				k2++;
			}
			char buf[256];
			char buf2[256];
			char output[256];
			char outdir[256];
			snprintf(buf, 256, "./%d_%d/%d.out",k,n,i);
			snprintf(buf2, 256,"./%d_%d/%d.out",k2,n2,i);

			n2 = n2 /2;
			k2++;

			snprintf(outdir, 256,"./%d_%d",k2,n2);
			mkdir(outdir,S_IRUSR | S_IWUSR | S_IXUSR);
			snprintf(output, 256,"./%d_%d/%d.out",k2,n2,i);
			FILE* firstfile = fopen(buf, "r");
			FILE* secondfile = fopen(buf2, "r");
			FILE* outputfile = fopen(output, "w");
			if(!firstfile)
			{
				fprintf(stderr, "Unable to open %s for reading", buf);
				exit(-1);
			}else if(!secondfile)
			{
				fprintf(stderr,"Unable to open %s for reading", buf2);
				exit(-1);
			}else if(!outputfile)
			{
				fprintf(stderr, "Unable to open %s for writing",output);
				exit(-1);
			}


			int mergefile_out = merge_files(firstfile,secondfile,outputfile,max_ngram_string_length);
			fclose(firstfile);
			fclose(secondfile);
			fclose(outputfile);
			if(mergefile_out)
			{
				fprintf(stderr, "Failed to merge files %s and %s\n",buf,buf2);
				exit(-1);
			}
			n = n2;
			k = k2;
		}
	}
	return std::pair<int,int>(n,k);
}


long long int analyze_ngrams(unsigned int ngramsize,FILE* infile,FILE* outfile)
{
    //Initialization:
	n_gram_size = ngramsize;
	#pragma omp flush(n_gram_size)
	max_ngram_string_length = (ngramsize * (MAX_WORD_SIZE + 1)) + 1;
	init_permanent_malloc(&swapDicts);
	remove("./processing");
	mkdir("./processing",S_IRUSR | S_IWUSR | S_IXUSR);
	chdir("./processing");
	lexicon = new Dict;
	prev_lexicon = new Dict;
	long long int totalwords = 0;
	word_list my_n_words;
	long long int count = 0;	
    	UFILE* f = u_fadopt(infile, "UTF8", NULL);
	//We use this to normalize the unicode text. See http://unicode.org/reports/tr15/ for details.
	UErrorCode e = U_ZERO_ERROR;
	const UNormalizer2* norm = unorm2_getNFKDInstance(&e);
	if(!U_SUCCESS(e))
		fprintf(stderr,"This is also a bug that needs to be fixed\n");

    //Stage 1: We get n-grams and mark them in paralell

	int state = 1;
	volatile int buffercount = -1;
	#pragma omp parallel 
	{
	#pragma omp master
	{
       		//shared(nextstring,f,norm,state,currentstring,stderr,count,totalwords,buffercount,current_page_group,my_n_words)
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
						//Here we merge the buffers that are already on disk.
						#pragma omp taskwait //This works taskwait waits for direct children of the master thread.
						if((buffercount > 0) && (buffercount % 2))
						{
								#pragma omp task firstprivate(buffercount)
								mergeNewBuffers(buffercount);
						}
				}
			}
			#pragma omp taskwait
			state = fillABuffer(f,totalwords,norm,my_n_words,count);
		}
	}
	}	
	
	//We've done all our reading to disk, let's close the input file.
	u_fclose(f);
	//Once we reach here we still have one buffer that needs to be written to disk.
	
	fprintf(stderr,"Writing out last buffer \n");
	buffercount++;
	writeBufferToDisk(buffercount,current_page_group,lexicon);
	int n = buffercount;
	int k = 0;
	std::pair<int,int> result = doLastMerge(n,k,buffercount);
	n = result.first;
	k = result.second;
	
	//At this point we have 256 input files that can be combined.
	for(int i = 0; i< 256; i++)
	{
		char buf[256];
		snprintf(buf,256, "./%d_%d/%d.out",k,n,i);
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
int getnextngram(UFILE* f,long long int &totalwords,const UNormalizer2* n,word_list &my_n_words,myNGram &ngram)
{
	int retval;
#pragma omp critical
{
	UChar* word;
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

