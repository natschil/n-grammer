#include "search.h"

using namespace std;



void do_search(map<unsigned int,Metadata> &metadatas,vector<string> arguments)
{
	if(!arguments.size())
	{
		cerr<<"search: Missing argument, please enter an argument like \"foo * bar baz\" as a search string"<<endl;
		exit(-1);
	}
	if(arguments.size() > 2)
	{
		cerr<<"search: Too many arguments."<<endl;
		exit(-1);
	}
	bool is_pos = false;
	if(arguments.size() == 2)
	{
		if(arguments[1] == "pos")
			is_pos = true;
		else
		{
			cerr<<"search: Unknown argument: "<<arguments[1]<<endl;
			exit(-1);
		}
	}

	stringstream argumentstream(arguments[0]);
	string cur("");

	//word positions that are part of the search string
	size_t ngramsize = 0;
	vector<unsigned int> known_words;
	vector<string> tokenized_search_string;

	//We tokenize the search string by spaces, and initialize the three variables above.
	while(getline(argumentstream,cur,' '))
	{
		tokenized_search_string.push_back(cur);						
		if(cur != "*")
		{
			known_words.push_back(ngramsize);
		}
		ngramsize++;
	}

	//We check if we have metadata for the index of that n-gram size
	if(metadatas.find(ngramsize) == metadatas.end())
	{
		cerr<<"search: Unable to find metadata for ngrams of size "<<ngramsize<<endl;
		exit(-1);
	}
	Metadata &relevant_metadata = metadatas[ngramsize];

	//We do a simple and unefficient search for the index that matches with the relevant permutation of known and unknown strings
	vector<unsigned int> search_index_to_use;	
	size_t matches = 0;

	for(set<vector<unsigned int> >::iterator itr = relevant_metadata.indices.begin(); itr != relevant_metadata.indices.end();itr++)
	{
		size_t counter = 0;
		for(vector<unsigned int>::const_iterator j = (*itr).begin(); j != (*itr).end(); j++)
		{
			if(find(known_words.begin(),known_words.end(),*j) != known_words.end())
				counter++;
			else break;
		}
		if(counter > matches)
		{
			search_index_to_use = *itr;
			matches = counter;
		}
	}
	if(!matches || (matches != known_words.size()))
	{
		cerr<<"search: No indexes found that could do this type of search."<<endl;
		exit(-1);
	}
	//We output a little bit of info before we start the search
	cerr<<"search: Searching with "<<matches<<" matches using index by";

	for(size_t i = 0; i < search_index_to_use.size(); i++)
	{
		cerr<<"_";
		cerr<<search_index_to_use[i];
	}
	cerr<<endl;

	//We make a string which has the filename of the index:
	string index_filename = relevant_metadata.folder_name + "/by";
	stringstream index_filename_stream(index_filename);
	index_filename_stream.seekp(0, std::ios::end);
	for(size_t i = 0; i< search_index_to_use.size(); i++)
	{
		index_filename_stream<<"_";
		index_filename_stream<<search_index_to_use[i];
	}
	index_filename_stream<<"/";
	index_filename_stream<<"0.out";
	index_filename= index_filename_stream.str();

	searchable_file* index_file = new searchable_file(index_filename);

	//We build a search string in the order that the index is in.
	string tosearchfor = string("");
	for(size_t i = 0; i< matches; i++)
	{
		if(i != 0)
			tosearchfor += " ";
		tosearchfor += tokenized_search_string[search_index_to_use[i]];
	}
	if(matches == search_index_to_use.size())
	{
		tosearchfor += "\t";
	}else
		tosearchfor += " ";


	//We do the actual search
	
	vector<string> results;
	index_file->search(tosearchfor, results);

	
	for(size_t i = 0; i < results.size(); i++)
	{
		string current_ngram = results[i];

		//We rearrange all of the strings we've found to be in the order that they are in the text.
		stringstream current_ngram_stream(current_ngram);
		vector<string> current_ngram_vector;
		stringstream current_word;	
		int c;
		while(1)
		{
			c = current_ngram_stream.get();
			if((c == ' ') || (c == '\t') || (c == EOF))
			{
				current_ngram_vector.push_back(current_word.str());
				current_word.str("");
				current_word.clear();
				if(c == EOF)
					break;
			}else
			{
				current_word<<(char)c;
			}
		}
		for(size_t i=0; i<search_index_to_use.size(); i++)
		{
			if(i)
				cout<<" ";
			for(size_t j = 0; j < search_index_to_use.size(); j++)
			{
				if(search_index_to_use[j] == i)
				{
					cout<<current_ngram_vector[j];
					break;
				}
			}
		}
		cout<<"\t"<<current_ngram_vector[ngramsize]<<"\n";
	}

	delete index_file;
	return;		
}
