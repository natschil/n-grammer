#include "ngram.counting.h"

using namespace std;
using namespace icu;

letterDict *lexicon = NULL;
//pthread_t writerthread;
volatile int itrcount = 0;
int n_gram_size = 0;

static void mark_ngram_occurance(myUString new_ngram);
static void swapDicts();
static void* writeDicts(void*);
void writeDict(letterDict &sublexicon,FILE* outfile);
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
			myUString s = this->front();	
			free(s.str);
			this->pop_front();
			this->size--;
		}
		this->finalstring_length = 0;
		this->std::deque<myUString>::clear();
		return;
	}

	void pop_font()
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

	myUString join_as_ngram(int *retval)
	{
		myUString result;
		result.str = (UChar*) permanently_malloc(this->finalstring_length * sizeof(*result.str),retval);
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

	int size;
	int finalstring_length;
	UChar* space;
};

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
	letterDict* old_dict = lexicon;
	//lexicon = (letterDict*)malloc(sizeof(Dict));
	lexicon = new Dict;
	/*
	for(int i = 0; i< 256;i++)
	{
		lexicon[i] = letterDict();
	}
	*/
	/*
	if(!safe_to_switch)
		pthread_join(writerthread,NULL); //Wait for potential old writer thread to finish.

	pthread_create(&writerthread,NULL,&writeDicts,(void*) old_dict);
	*/
	return;	

}


static void *writeDicts(void* input)
{
	Dict* dict_to_write = (Dict*) input;
	//TODO: Paralellize this
	char buf[256];//Big enough.
	if(snprintf(buf,256,"mkdir -p %d",itrcount) >= 256)
	{
		//This should never happen.
		fprintf(stderr,"Error, poor program design");
		exit(-1);
	}
	if(system(buf))
	{
		//This should also never happen
		fprintf(stderr,"Unable to create directory");
		exit(-1);
	}

	for(int i = 0; i < 256; i++)
	{
		if(snprintf(buf,256,"./%d/%d.out",itrcount,i) >= 256)
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
		writeDict((*dict_to_write)[i],cur);
		fflush(cur);
		fclose(cur);
		(*dict_to_write)[i].clear();
	}
	//We tell memory_management.h that we've finished writing out this buffer.
	safe_to_switch = 1;
	itrcount++;
	return NULL;
}


void writeDict(letterDict &D,FILE* outfile)
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
	size_t firstchar = (size_t)((char) new_ngram.str[0] + 128);
	letterDict &lD = lexicon[firstchar];

	if(lD.find(new_ngram) != lD.end())
		lD[new_ngram]++;
	else lD[new_ngram] = 1;

	return;
}


long long int analyze_ngrams(unsigned int ngramsize,FILE* file)
{
    //Initialization:
	n_gram_size = ngramsize;
	init_permanent_malloc();
	system("rm -r ./processing > /dev/null 2>/dev/null");	
	system("mkdir ./processing");
	chdir("./processing");
	lexicon = new Dict;
	long long int totalwords = 0;
	word_list my_n_words;
	int count = 0;	
    	UFILE* f = u_fadopt(file, "UTF8", NULL);
	//We use this to normalize the unicode text. See http://unicode.org/reports/tr15/ for details.
	UErrorCode e = U_ZERO_ERROR;
	const UNormalizer2* n = unorm2_getNFKDInstance(&e);
	if(!U_SUCCESS(e))
		fprintf(stderr,"This is also a bug that needs to be fixed\n");
    //Stage 1: We get n-grams and mark them in paralell

	myUString currentstring;
        int state = getnextngram(f,totalwords,n,my_n_words,currentstring);
	while(1)
	{
		if(state == 0) //End of file
			break;
		else if(state == -1) //Buffer is full
			break;
		#pragma omp parallel
		{
			myUString nextstring;
			#pragma omp task	
			{
				state = getnextngram(f,totalwords,n,my_n_words,nextstring);
			}
			#pragma omp task
			{
				#pragma omp flush(currentstring)
				mark_ngram_occurance(currentstring);
			}
			#pragma omp taskwait
			#pragma omp flush(state)	
			currentstring = nextstring;
		}
	}

	u_fclose(f);
	printf("%lld",totalwords);
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
	
		int retval = 0;
		str = my_n_words.join_as_ngram(&retval);
		return retval;
	}
}

