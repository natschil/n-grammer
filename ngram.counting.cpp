#include "ngram.counting.h"
using namespace std;
using namespace icu;


//Returns 0 if there is not word.
int getnextword(UnicodeString &s,UFILE* f)
{

    int wordlength = 0; 
    int first_is_hyphen;
    UChar word[42]; //words may not be longer than 40 characters. It is marked static because I think that might improve performance
    //The size is 42 so that it can hold a word of length 41 which is longer than 40 characters and will hence trigger it to be ignored in 
    //The next code.
    UChar character;

    //See http://icu-project.org/apiref/icu4c/ustdio_8h.html#a48b9be06c611643848639fa4c22a16f4
    for(wordlength = 0;((character = u_fgetc(f)) != U_EOF);)
    {
	//We now check whether or not the character is a whitespace character:
	if(u_isUWhiteSpace(character))
	{
		if(!wordlength) //We ignnore preceding whitespace.
			continue;
		else //We have reached the end of the word
			break;
	}else if(wordlength < 41) //After 41 characters we ignore all non-whitespace stuff.
	{
		 if(u_isUAlphabetic(character))
		{
			//We do uppercase to lowercase conversion here.
			word[wordlength] = u_tolower(character);
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
		}else if(u_ispunct(character))
		{
			continue;
		}else
		{
			continue; 
		}
	}
    }

    word[wordlength+1] = '\0';
    if(first_is_hyphen && (wordlength == strlen("END.OF.DOCUMENT---")))
    {
	s = UnicodeString(word,wordlength);
	if(s == "END.OF.DOCUMENT")
	{
		return 41;
	}
    }

    if(wordlength)
    {
	s = UnicodeString(word,wordlength);

	//At this point we have a unicode string. However, normalization issues are still present.
	//Note that we nevertheless return the string because it is more efficient to normalize while we merge.
	return wordlength;
   }else
   {
	return 0;
  }
}

void writeDict(Dict& D, const double corpussize)
{
  UnicodeString word;
  double freq;
  printf("Word\tRawFreq\tFreqPerMillion\n");
  for (Dict::iterator i = D.begin(); i != D.end(); i++) {
    string myword;
    (i->first).toUTF8String(myword);
    freq = i->second;
    if ( freq >3) {
      printf("%s\t%lld\t%0.8g\n",&myword[0],(long long int)freq,(double)(freq*1000000.0)/corpussize);
    }
  }
}


Dict &mark_ngram_occurance(Dict &lexicon, UnicodeString new_ngram)
{
	//TODO: Think about the fact that this has to lookup new_ngram in lexicon twice.
	if(lexicon.find(new_ngram) != lexicon.end())
		lexicon[new_ngram]++;
	else lexicon[new_ngram] = 1;

	return lexicon;
}


double analyze_ngrams(Dict &lexicon,unsigned int ngramsize,FILE* file)
{
	int count = 0;
	int totalwords = 0;
    	UFILE*  f = u_fadopt(file, "UTF8", NULL);


	//We use this to normalize the unicode text. See http://unicode.org/reports/tr15/ for details.
	UErrorCode e = UErrorCode();
	const Normalizer2* nfkc = Normalizer2::getInstance(NULL,"nfkc",UNORM2_COMPOSE,e);

	//A deque of the previous ngramsize-1 words in the text.
	word_list my_n_words;  

	while(1)
	{
		if(count % 1000000 == 0) 
		{
			fprintf(stderr,"%ld",(long int) count);
			fflush(stderr);
		}

		count++;
		UnicodeString word;
		int wordlength = getnextword(word,f);

		if(!wordlength) //We've reached the end of the file
		{
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
		if(wordlength > 40)
		{
			my_n_words.clear();
			continue;
		}

		totalwords++;

		//We add the word to our list.
		my_n_words.push_back(word);
		if(my_n_words.size() < ngramsize)
		{
			continue;
		}

		//So that my_n_words does not grow infinitely.
		if(my_n_words.size() > ngramsize)
		{
			my_n_words.pop_front();
		}

		//We join the words, TODO: Optimize this a lot.
		UnicodeString finalstring("");
		int first = 1;
		for(word_list::iterator j = my_n_words.begin(); j != my_n_words.end();j++)
		{
			//TODO:Check for errors.
			UErrorCode e = UErrorCode();
			if(!first)
			{
				nfkc->normalizeSecondAndAppend(finalstring,UnicodeString(" "),e);	
				nfkc->normalizeSecondAndAppend(finalstring,*j,e);
			
			}
			else
			{
				nfkc->normalizeSecondAndAppend(finalstring,*j,e);
			}
			
			first = 0;
		}

		mark_ngram_occurance(lexicon,finalstring);	

	}
	
	u_fclose(f);
	return totalwords;
}


