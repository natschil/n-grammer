#include "ngram.counting.h"

using namespace std;
using namespace icu;

Dict &mark_ngram_occurance(Dict &lexicon, UChar* new_ngram);

bool uchar_cmp::operator()(myUString first, myUString second)
{
	int diff = first.length - second.length;
	if(!diff)
	{
		return (u_strcmp(first.str,second.str)) < 0;
	}else
		return diff < 0;
}

myUString::~myUString(){return;};

static void* pages[1000000] = {NULL};
static size_t num_pages = 0;
static size_t current_page_filled = 0;

//TODO: Look at http://wiki.debian.org/Hugepages and whether it might be relevant.
void init_permanent_malloc()
{
	if(!(pages[0] = malloc(getpagesize())))
	{
		fprintf(stderr,"Error allocating memory");
	}
	num_pages = 1;
	current_page_filled = 0;
}
void* permanently_malloc(size_t numbytes)
{
	if(( current_page_filled += numbytes )<(size_t) getpagesize())
	{
		return pages[num_pages-1] + (current_page_filled - numbytes);
	}else
	{
		if(!(pages[num_pages++] =  malloc(getpagesize())))
			fprintf(stderr,"Error allocating memory");
		current_page_filled = 0;
		return permanently_malloc(numbytes);
	}
}

void free_all_pages()
{
	while(num_pages--)
	{
		free(pages[num_pages]);
	}
	return;
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

void writeDict(Dict& D, const double corpussize,size_t ngramsize)
{
  UnicodeString word;
  double freq;
  printf("Word\tRawFreq\tFreqPerMillion\n");
  for (Dict::iterator i = D.begin(); i != D.end(); i++) {
    int32_t wordlength;
    UErrorCode e = U_ZERO_ERROR; //This means no error
    char* myword = (char*) malloc(MAX_WORD_SIZE * (ngramsize + 1) + 1 ); //TODO: Fix this.
    u_strToUTF8(myword,(ngramsize + 1) * MAX_WORD_SIZE+1,&wordlength, i->first.str,i->first.length,&e);
    if(!U_SUCCESS(e))
    {
	fprintf(stderr,"There was an error converting a string to UTF8\n");
	//return;
    }

    freq = i->second;
    if ( freq >3) {
      printf("%s\t%lld\t%0.8g\n",myword,(long long int)freq,(double)(freq*1000000.0)/corpussize);
    }
  }
  free_all_pages();
}


Dict &mark_ngram_occurance(Dict &lexicon, myUString new_ngram)
{
	//TODO: Think about the fact that this has to lookup new_ngram in lexicon twice.
	if(lexicon.find(new_ngram) != lexicon.end())
		lexicon[new_ngram]++;
	else lexicon[new_ngram] = 1;

	return lexicon;
}


double analyze_ngrams(Dict &lexicon,unsigned int ngramsize,FILE* file)
{
	init_permanent_malloc();
	int count = 0;
	int totalwords = 0;
    	UFILE*  f = u_fadopt(file, "UTF8", NULL);
	UErrorCode e = U_ZERO_ERROR;
	UChar* space = (UChar*) malloc(sizeof(*space));
	int32_t spacelength = 0;
	u_strFromUTF8(space,1,&spacelength," ",-1,&e);
	if(!U_SUCCESS(e) || spacelength != 1)
	{
		fprintf(stderr,"This is a bug that needs to be fixed\n");

	}
		

	//We use this to normalize the unicode text. See http://unicode.org/reports/tr15/ for details.
	e = U_ZERO_ERROR;
	const UNormalizer2* nfkc = unorm2_getNFKDInstance(&e);
	if(!U_SUCCESS(e))
	{
		fprintf(stderr,"This is also a bug that needs to be fixed\n");
	}

	//A deque of the previous ngramsize-1 words in the text.
	word_list my_n_words;  
	size_t my_n_words_size = 0;

	int finalstring_length = 0;
	while(1)
	{
		if(count % 1000000 == 0) 
		{
			fprintf(stderr,"%ld\n",(long int) count);
			fflush(stderr);
		}

		count++;
		UChar* word;
		int wordlength = getnextword(word,f,nfkc);

		if(!wordlength) //We've reached the end of the file
		{
			free(word);
			break;
		}

		//getnextword checks whether the word is equal to ---END.OF.DOCUMENT---
		/*
 		if(word == UnicodeString("---END.OF.DOCUMENT---"))
		{
			my_n_words.clear();
			continue;
		}
		*/
		if(wordlength > MAX_WORD_SIZE)
		{
			free(word);
			myUString s;
			while(my_n_words_size)
			{
				s = my_n_words.front();
				free(s.str);
				my_n_words.pop_front();
				my_n_words_size--;
			}
			finalstring_length = 0;
			continue;
		}

		totalwords++;

		//We add the word to our list.
		myUString newUString;
		newUString.str = word;
		newUString.length = wordlength;

		my_n_words.push_back(newUString);
		finalstring_length += wordlength;
		my_n_words_size++;

		if(finalstring_length) //To accomodate for the fact that there is an additional space.
			finalstring_length++;

		if(my_n_words_size < ngramsize)
		{
			continue;
		}

		//So that my_n_words does not grow infinitely.
		if(my_n_words_size > ngramsize)
		{
			finalstring_length -= my_n_words.front().length;
			finalstring_length--; //Because of the space
			free(my_n_words.front().str);
			my_n_words.pop_front();
			my_n_words_size--;
		}

		//We join the words, TODO: Optimize this a lot.
		myUString finalstring;
		finalstring.str = (UChar*) permanently_malloc(sizeof(*finalstring.str) * finalstring_length);
		finalstring.length = finalstring_length;

		int first = 1;
		UChar* ptr = finalstring.str;
		for(word_list::iterator j = my_n_words.begin(); j != my_n_words.end();j++)
		{
			myUString cur = *j;
			if(first)
			{
				memcpy(ptr,cur.str,cur.length * sizeof(UChar));
			}else
			{
				memcpy(ptr,space,sizeof(UChar));
				ptr++;
				memcpy(ptr,cur.str,sizeof(UChar) * cur.length);
			}
			ptr += cur.length;
			first = 0;
		}
		*ptr = (UChar) 0;

		//mark_ngram_occurance(lexicon,finalstring);	

	}
	
	u_fclose(f);
	free(space);
	return totalwords;
}


