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

using namespace std;

typedef map<string, double> Dict;
typedef deque<string> word_list;

vector<string> &split(istream &is, char delim, vector<string> &elems) {
    string mystring;
    getline(is,mystring);
    stringstream ss(mystring);
    string word;
    while(getline(ss,word,delim)){
        elems.push_back(word);
    }
    return elems;
}

vector<string> split(istream &s, char delim) {
    vector<string> elems;
    return split(s, delim, elems);
}

void writeDict(Dict& D, const double corpussize)
{
  string word;
  double freq;
  cout << "Word\tRawFreq\tFreqPerMillion\n";
  cout.precision(8);
  for (Dict::iterator i = D.begin(); i != D.end(); i++) {
    word = i->first;
    freq = i->second;
    if ( freq >3) {
      cout << word << "\t" << freq  << "\t" << (freq*1000000.0)/corpussize << endl;
    }
  }
}

static bool IsNonAlpha(const char& c)
{
  return !isalpha(c);
}


Dict &mark_ngram_occurance(Dict &lexicon, string new_ngram)
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

	word_list my_n_words;  
	do
	{
		vector<string> elems;		

		if(count % 1000000 == 0) //TODO: Reset this to the value it was before
		{
			cerr<<count<<" " << flush;
		}
		count++;
		elems = split(cin,' ');
		//Let's normalize the words:
		for(vector<string>::iterator i =elems.begin(); i!=elems.end();++i)
		{
			string word = *i;
	
			if(!word.size() ||  (word.size() > 40) || (word == "---END.OF.DOCUMENT---"))
			{
				continue;
			}

			word.erase(remove_if(word.begin(),word.end(),&IsNonAlpha),word.end());
			transform(word.begin(),word.end(),word.begin(),::tolower);

			if(!word.size())
				continue;

			totalwords++;

			//We add the word to our list.
			my_n_words.push_back(word);
			if(my_n_words.size() < ngramsize)
				continue;

			if(my_n_words.size() > ngramsize) //We can't do this every time because else we forget the first case.
				my_n_words.pop_front();
	
			//We join the words, TODO: Optimize this a lot.
			string finalstring = string("");
			int first = 1;
			for(word_list::iterator j = my_n_words.begin(); j != my_n_words.end();j++)
			{
				if(!first)
					finalstring += (" " + *j);
				else
					finalstring += *j;
				
				first = 0;
			}
	
			mark_ngram_occurance(lexicon,finalstring);	
		}
	}while(cin);
	
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
