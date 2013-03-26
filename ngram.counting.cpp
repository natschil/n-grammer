// Count word n-grams in a corpus.
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <deque>
#include <string>
#include <algorithm>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#define U_CHARSET_IS_UTF8 1
#include <unicode/utf.h>
#include <unicode/uchar.h>
#include <unicode/unistr.h>
#include <unicode/normalizer2.h>
#include <unicode/ustdio.h>

using namespace std;


typedef map<UnicodeString, double> Dict;
typedef deque<UnicodeString> word_list;

typedef icu::UnicodeString UnicodeString;


UnicodeString* getnextword(FILE* istream)
{

    int wordlength = 0; 

    vector <UChar32> word;
    UFILE*  f = u_fadopt(istream, "UTF-8", NULL);
    UChar32 character;
    //See http://icu-project.org/apiref/icu4c/ustdio_8h.html#a48b9be06c611643848639fa4c22a16f4
    while(((character = u_fgetcx(f)) != U_EOF))
    {
	//We now check whether or not the character is a whitespace character:
	if(u_isUWhiteSpace(character))
	{
		if(!wordlength) //We ignnore preceding whitespace.
			continue;
		else //We have reached the end of the word
			break;
	}else
	{
		if(u_isUAlphabetic(character))
		{
			wordlength++;
			word.push_back(character);
		}else if(u_hasBinaryProperty(character, UCHAR_HYPHEN))
		{
			//We treat hyphens like we treat normal characters..
			wordlength++;
			word.push_back(character);
		}else//We ignore it.
		{

		}
	}

    }

    if(wordlength)
    {
	UnicodeString* result = new UnicodeString((int32_t) wordlength,'\0',0);

	while(wordlength--)
	{
		result += word.back();
		word.pop_back();
	}

	//At this point we have a unicode string. However, normalization issues are still present.
	//Note that we nevertheless return the string because it is more efficient to normalize while we merge.
	
	return result;
   }else
    return NULL;
}

void writeDict(Dict& D, const double corpussize)
{
  UnicodeString word;
  double freq;
  cout << "Word\tRawFreq\tFreqPerMillion\n";
  cout.precision(8);
  for (Dict::iterator i = D.begin(); i != D.end(); i++) {
    string myword;
    (i->first).toUTF8String(myword);
    freq = i->second;
    if ( freq >3) {
      cout << myword << "\t" << freq  << "\t" << (freq*1000000.0)/corpussize << endl;
    }
  }
}

static bool IsNonAlpha(const char& c)
{
  return !isalpha(c);
}


Dict &mark_ngram_occurance(Dict &lexicon, UnicodeString new_ngram)
{
	//TODO: Think about the fact that this has to lookup new_ngram in lexicon twice.
	if(lexicon.find(new_ngram) != lexicon.end())
		lexicon[new_ngram]++;
	else lexicon[new_ngram] = 1;

	return lexicon;
}


double analyze_ngrams(Dict &lexicon,unsigned int ngramsize)
{
	int count = 0;
	int totalwords = 0;

	UErrorCode e;



	word_list my_n_words;  
	do
	{
		if(count % 1000000 == 0) //TODO: Reset this to the value it was before
		{
			cerr<<count<<" " << flush;
		}
		count++;
		//Let's normalize the words:
		UnicodeString* word = getnextword(stdin);
		if(!word)
			break;

/*	 TODO: Implement the checks below.
			if(!word.size() ||  (word.size() > 40) || (word == "---END.OF.DOCUMENT---"))
			{
				continue;
			}


			word.erase(remove_if(word.begin(),word.end(),&IsNonAlpha),word.end());
			transform(word.begin(),word.end(),word.begin(),::tolower);

			if(!word.size())
				continue;
*/

			totalwords++;

			//We add the word to our list.
			my_n_words.push_back(*word);
			if(my_n_words.size() < ngramsize)
				continue;

			if(my_n_words.size() > ngramsize) //We can't do this every time because else we forget the first case.
				my_n_words.pop_front();
	
			//We join the words, TODO: Optimize this a lot.
			UnicodeString finalstring;
			int first = 1;
			for(word_list::iterator j = my_n_words.begin(); j != my_n_words.end();j++)
			{
				UErrorCode e;
				if(!first)
				{
					//icu::Normalizer2::append(finalstring,UnicodeString(" "),e);	
					//icU::Normalizer2::apend(finalstring,*j);
				
				}
				else
				{
					//icu::Normalizer2::append(finalstring,*j,e);
				}
				
				first = 0;
			}
	
			mark_ngram_occurance(lexicon,finalstring);	
	}while(1);
	
	return totalwords;
}

int main (int argc, char* argv[])
{
  if(argc != 2)
  {
    cerr << "Usage: " << argv[0] << " N < input > output\n Where N is the size of the ngrams you want to count." << endl;
    exit(1);
  }

  char* number = argv[1];
  char* errptr;
  unsigned int ngramsize = (unsigned int) strtol(number, &errptr,10);
  if(!ngramsize || (errptr && *errptr))
  {
    cerr << "Usage: " << argv[0] << " N < input > output\n Where N is the size of the ngrams you want to count." << endl;
    exit(1);
  }

  Dict lexicon;
  double totalwords = analyze_ngrams(lexicon,ngramsize);
  writeDict(lexicon,totalwords);  
  return 0; 
}
