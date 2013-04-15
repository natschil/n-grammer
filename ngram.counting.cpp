#include "ngram.counting.h"

using namespace std;
using namespace icu;

letterDict *lexicon = NULL;
letterDict *prev_lexicon = NULL;
//pthread_t writerthread;
int n_gram_size = 0;
int max_ngram_string_length = 0;

static void mark_ngram_occurance(myUString new_ngram);
static void swapDicts();
static void* writeLetterDict(int buffercount,int number);
void writeLetterDictToFile(letterDict &sublexicon,FILE* outfile);
struct word_list;
int getnextngram(UFILE* f,long long int &totalwords,const UNormalizer2* n,word_list &my_n_words,myUString &str);

bool uchar_cmp::operator()(myUString first, myUString second)
{
	/*
	int diff = first.length - second.length;
	if(!diff)
	{
		return (u_strcmp(first.str,second.str)) < 0;
	}else
		return diff < 0;
		*/
	return u_strcmp(first.str,second.str) < 0; //Makes merging easier because we don't need to convert to UTF16 again..
}

myUString::~myUString(){return;};

struct word_list : public std::deque<myUString>
{
	word_list() : std::deque<myUString>()
	{
		this->size = 0;
		this->finalstring_length = 0;
		UErrorCode e = U_ZERO_ERROR;
		this->space = (UChar*) malloc(sizeof(*space));
		int32_t spacelength = 0;
		u_strFromUTF8(space,1,&spacelength," ",-1,&e);
		if(!U_SUCCESS(e) || spacelength != 1)
		{
			fprintf(stderr,"This is a bug that needs to be fixed\n");
	
		}
	}

	~word_list() 
	{
		free(this->space);
	}
	void clear()
	{
		while(this->size)
		{
			this->pop_front();
		}
		this->finalstring_length = 0;
		return;
	}

	void pop_front()
	{
		myUString cur = this->front();
		free(cur.str);
		this->size--;
		this->finalstring_length--;
		this->finalstring_length -=  cur.length;
		this->std::deque<myUString>::pop_front();
		return;
	}

	void push_back(myUString newstring)
	{
		if(finalstring_length)
			finalstring_length++;
		finalstring_length += newstring.length;
		this->size++;
		this->std::deque<myUString>::push_back(newstring);
	}

	myUString join_as_ngram(int *retval);

	int size;
	int finalstring_length;
	UChar* space;
};
myUString word_list::join_as_ngram(int *retval)
{
	myUString result;
	result.str = (UChar*) permanently_malloc((this->finalstring_length + 1) * sizeof(*result.str) ,retval);
	result.length = this->finalstring_length;

	int first = 1;
	UChar* ptr = result.str;
	for(word_list::iterator j = this->begin(); j != this->end();j++)
	{
		myUString cur = *j;
		if(first)
		{
			memcpy(ptr,cur.str,cur.length * sizeof(UChar));
		}else
		{
			memcpy(ptr,this->space,sizeof(UChar));
			ptr++;
			memcpy(ptr,cur.str,sizeof(UChar) * cur.length);
		}
		ptr += cur.length;
		first = 0;
	}
	*ptr = (UChar) 0;

	return result;
}
#define MAX_WORD_SIZE 40
//Sets s to point to a normalized UChar* with the next word from f. s is allocated using malloc(3), and normalized using n.
//Returns the size of f iff there is a next word.
//Returns 0 if there is no subsequent word in f. However, s is still freeable even then.
//Returns x so that x > MAX_WORD_SIZE if the contents of s should be ignored, but freed using free(3);
//Alphabetic characters from f are converted to their lowercase equivalents.
//Diacritics are left unmodified unless through normalization.
//Hyphen Characters are ignored unless they are within a word. Therefore an input of --X-ray will result in the word x-ray being returned
//Punctuation and Digit Characters are Ingored
//Any whitespace constitutes a word delimiter.
//Any other characters are ignored but cause a warning to be printed to stderr.

int getnextword(UChar* &s,UFILE* f,const UNormalizer2* n)
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
	    s = (UChar*) malloc(sizeof(*s));
	    *s = (UChar) 0;
	    return MAX_WORD_SIZE + 1;
    }
    word[wordlength+1] = '\0';
    if(first_is_hyphen && (wordlength == strlen("END.OF.DOCUMENT---")))
    {
	//TODO: Fix this truly ugly code.
	for(size_t itr = 0; itr < strlen("END.OF.DOCUMENT---");itr++)
	{
		if(((char) word[itr]) != "END.OF.DOCUMENT"[itr])
		{
			goto out;
		}
	}
	//TODO: Test whether the following works.
	s = (UChar*) malloc(sizeof(*s));
	*s = (UChar) 0;
	
	return MAX_WORD_SIZE + 1;
    }
    out:

    if(wordlength)
    {
	//At this point we have a unicode string. However, normalization issues are still present
	s = (UChar*) malloc(sizeof(*s) * MAX_WORD_SIZE);
	UErrorCode e = U_ZERO_ERROR;
	int retval =  unorm2_normalize(n,word,wordlength,s,MAX_WORD_SIZE,&e);
	if(!U_SUCCESS(e))
	{
		retval = MAX_WORD_SIZE + 1;
	}
	return retval;
   }else
   {
	s = (UChar*) malloc(sizeof(*s));
	*s = (UChar) 0;
	return 0;
  }
}

static void swapDicts()
{
	//HERE1
	letterDict *tmp = prev_lexicon;
	prev_lexicon = lexicon;
	lexicon = tmp;
	return;	

}


static void *writeLetterDict(int buffercount,int i)
{
	//TODO: Paralellize this
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
	FILE* cur = fopen(buf,"w");
	if(!cur)
	{
		fprintf(stderr,"Unable to open file %s", buf);
		exit(-1);
	}

	writeLetterDictToFile(prev_lexicon[i],cur);
	fflush(cur);
	fclose(cur);
	//HERE1 relies on the following step
	prev_lexicon[i].clear();
	//We tell memory_management.h that we've finished writing out this buffer.
	safe_to_switch = 1;
	return NULL;
}


void writeLetterDictToFile(letterDict &D,FILE* outfile)
{
  UnicodeString word;
  double freq;
	  for (letterDict::iterator i = D.begin(); i != D.end(); i++) {
	    int32_t wordlength;
	    UErrorCode e = U_ZERO_ERROR; //This means no error
	    char* myword = (char*) malloc(MAX_WORD_SIZE * (n_gram_size + 1) + 1 ); //TODO: Fix this.
	    u_strToUTF8(myword,(n_gram_size + 1) * MAX_WORD_SIZE+1,&wordlength, i->first.str,i->first.length,&e);
	    if(!U_SUCCESS(e))
	    {
		fprintf(stderr,"There was an error converting a string to UTF8\n");
		//return;
	    }
	
	    freq = i->second;
	    fprintf(outfile,"%s\t%lld\n",myword,(long long int)freq);
  }
}


static void mark_ngram_occurance(myUString new_ngram)
{
	//TODO: Think about the fact that this has to lookup new_ngram in lexicon twice.
	
	//We run a kind of bucket-sort which hopefully makes things slightly faster.
	size_t firstchar = (size_t) (unsigned char) new_ngram.str[0];
	letterDict &lD = lexicon[firstchar];

	if(lD.find(new_ngram) != lD.end())
		lD[new_ngram]++;
	else lD[new_ngram] = 1;

	return;
}


long long int analyze_ngrams(unsigned int ngramsize,FILE* infile,FILE* outfile)
{
    //Initialization:
	n_gram_size = ngramsize;
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

	myUString currentstring;
	myUString nextstring;
        int state = getnextngram(f,totalwords,norm,my_n_words,currentstring);
	int newstate = state;
	int buffercount = -1;
	while(1)
	{
		if(state == 0) //End of file
			break;
		#pragma omp parallel sections
		{
			//Here we fill one half of the memory used with n-grams.
			#pragma omp section	
			{
				newstate = 1;
				while(1)
				{
					if(newstate != 1)
					{
						#pragma omp flush(state,newstate)
						state = newstate;
						#pragma omp flush(state)
						break;
					}
					#pragma omp parallel sections
					{
						#pragma omp section
						{
							newstate = getnextngram(f,totalwords,norm,my_n_words,nextstring);
						}
						#pragma omp section
						{
							mark_ngram_occurance(currentstring);
						}
					}
					currentstring = nextstring;
					count++;
					if((count % 1000000 == 0))
					{
						printf("%lld\n",(long long int) count);
					}
				}
			}
			//Here we concurrently write out  the n-grams from the other half of memory.
			#pragma omp section
			{
				if(state == -1)
				{
					#pragma omp flush(safe_to_switch)
					safe_to_switch = 0;
					#pragma omp flush(safe_to_switch)
					printf("Switching buffers\n");
					#pragma omp parallel for
					for(int i = 0; i< 256;i++)
					{
						writeLetterDict(buffercount+1,i);
					}
					#pragma omp flush(safe_to_switch)
					safe_to_switch = 1;
					#pragma omp flush(safe_to_switch)
					state = 1;
					#pragma omp flush(state)

					#pragma omp flush(buffercount)
					buffercount++;
					#pragma omp flush(buffercount)

				}
			}
			//Here we merge the buffers that are already on disk.
			#pragma omp section
			{
				#pragma omp flush(buffercount)
				if((buffercount > 0) && (buffercount % 2))
				{
					#pragma omp parallel for
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
			}
		}
		#pragma omp flush
	}
	//We've done all our reading to disk, let's close the input file.
	u_fclose(f);
	//Once we reach here we still have one buffer that needs to be written to disk.
	
	printf("Writing out last buffer \n");
	swapDicts();
	#pragma omp parallel for
	for(int i = 0; i< 256;i++)
	{
		writeLetterDict(buffercount+1,i);
	}
	buffercount++;
	int n = buffercount;
	int k = 0;
	#pragma omp parallel for
	for(int i = 0; i< 256;i++)
	{
		while(n)
		{
			int n2 = n - 1 ;
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

			k++;
			n = n2 / 2;

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
	//At this point we have 256 input files that can be combined.
	for(int i = 0; i< 256; i++)
	{
		char buf[256];
		snprintf(buf,256, "./%d_%d/%d.out",k,n,i);
		FILE* cur = fopen(buf,"r");
		int curchar;
		while((curchar = fgetc(cur)) != EOF)
		{
			fputc((char) curchar,outfile);
		}
	}
	return totalwords;
}

//Returns 1 for simpl having fetched the next n-gram
//Returns 0 if there is nothing more to fetch
//Returns -1 if we need to switch buffers.
int getnextngram(UFILE* f,long long int &totalwords,const UNormalizer2* n,word_list &my_n_words,myUString &str)
{
	UChar* word;
	while(1)
	{
		int wordlength = getnextword(word,f,n);
		if(!wordlength) //We've reached the end of the file
		{
			free(word);
			return 0;
		}else if(wordlength > MAX_WORD_SIZE)
		{
			my_n_words.clear();
			continue;
		}
		totalwords++;
		//We add the word to our list.
		myUString newUString;
		newUString.str = word;
		newUString.length = wordlength;
	
		my_n_words.push_back(newUString);
	
		if(my_n_words.size < n_gram_size)
			continue;
		else if(my_n_words.size > n_gram_size) //So that my_n_words does not grow infinitely.
		{
			my_n_words.pop_front();
		}
	
		int retval = 1;
		str = my_n_words.join_as_ngram(&retval);
		return retval;
	}
}

