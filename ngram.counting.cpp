// Count word n-grams in a corpus.
//
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
using namespace icu;


typedef map<UnicodeString, double> Dict;
typedef deque<UnicodeString> word_list;

typedef UnicodeString UnicodeString;


UnicodeString getnextword(UFILE* f,int &over)
{

    int wordlength = 0; 

    deque <UChar32> word;
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
	}else if(u_isUAlphabetic(character))
	{
		wordlength++;
		//We do uppercase to lowercase conversion here.
		word.push_back(u_tolower(character));
	}else if(u_hasBinaryProperty(character, UCHAR_HYPHEN) || u_hasBinaryProperty(character, UCHAR_DIACRITIC ))
	{
		if(wordlength)
		{
			//We treat hyphens like we treat normal characters..
			wordlength++;
			word.push_back(character);
		}
	}else if(u_ispunct(character))
	{
		continue;
	}else
	{
		continue; 
	}

    }

    if(wordlength)
    {
	UnicodeString result((int32_t) wordlength,'\0',0);

	while(wordlength--)
	{
		result += word.front();
		word.pop_front();
	}

	//At this point we have a unicode string. However, normalization issues are still present.
	//Note that we nevertheless return the string because it is more efficient to normalize while we merge.
	over = 0;	
	return result;
   }else
   {
	over = 1;
	return UnicodeString(""); //Because we have to return something.
  }
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

	while(1){
		if(count % 1000000 == 0) 
		{
			cerr<<count<<" " << flush;
		}

		count++;
		int over = 0;
		UnicodeString word = getnextword(f,over);
		if(over) //This means we've reached the end of the file.
			break;

		if(!word.length()) //Should never happen in practice
		{
			cerr<<"There seems to be a bug somewhere"<<endl<<flush;
			continue;
		}
 		if(word == UnicodeString("---END.OF.DOCUMENT---"))
		{
			my_n_words.clear();
			continue;
		}
		if(word.length() > 40)
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

int main (int argc, char* argv[])
{
  if((argc == 1) || (argc > 3))
  {
    cerr << "Usage: " << argv[0] << " N < input > output\n Where N is the size of the ngrams you want to count." << endl;
    cerr << "OR" <<argv[0] << " N input > output"<<endl;
    exit(1);
  }

  char* number = argv[1];
  char* errptr;
  unsigned int ngramsize = (unsigned int) strtol(number, &errptr,10);
  if(!ngramsize || (errptr && *errptr))
  {
    cerr << "The first parameter is not a valid number"<<endl;
    cerr << "Usage: " << argv[0] << " N < input > output\n Where N is the size of the ngrams you want to count." << endl;
    cerr << "OR" <<argv[0] << " N input > output"<<endl;
    exit(1);
  }

  Dict lexicon;
  FILE* f = NULL;
  if(argc == 3)
  {
	f = fopen(argv[2],"r");
	if(!f)
	{
		cerr<<"Could not open file " <<argv[2]<<endl;
		exit(1);
	}

  }else
	f = stdin;

  
  double totalwords = analyze_ngrams(lexicon,ngramsize,f);
  writeDict(lexicon,totalwords);  
  return 0; 
}
