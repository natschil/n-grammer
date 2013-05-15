#include "search.h"

using namespace std;


static void single_search(const char* mmaped_index, off_t mmaped_index_size,map<string,off_t> search_tree,const char* search_string,size_t search_string_length,vector<string> &results)
{

	/*
	map<string,off_t>::iterator itr = search_tree.lower_bound(string(search_string));
	if(itr == search_tree.end())
	{
		return;
	}
	int64_t upper = (*itr).second;
	itr++;
	if(itr == search_tree.end())
	{
		return;
	}
	int64_t lower = (*itr).second;
	int64_t i;
	*/
	int64_t upper = mmaped_index_size - 1;
	int64_t lower = 0;
	int64_t i;

	stringstream foundstrings;
	while(1)
	{
		if(upper<lower)
		{
			cerr<<"No matches found"<<endl;
			break;
		}else
		{
			i = (upper + lower)/2;
			while((i >= lower) && (mmaped_index[i] != '\n'))
				i--;
			//Go to the first thing after the \n, or to the lowest thing.
			i++;
			uint64_t string_length = 0;

			search_tree[string(mmaped_index +i,search_string_length)] = i;
			int cmp = strncmp(mmaped_index + i,search_string,search_string_length);
			if(cmp == 0)
			{
				int64_t  itr;
				for(itr = i; itr<mmaped_index_size ;itr++)
				{
					if((mmaped_index[itr] == '\n') && (itr < (mmaped_index_size -1)))
					{
						results.push_back(foundstrings.str());
						foundstrings.str("");
						itr++;
						if(strncmp(mmaped_index +itr, search_string,search_string_length))
						{
							break;
						}
						itr--; 
					}
					foundstrings<<mmaped_index[itr];
				}
				while(i)
				{
					for(itr = i - 2; itr > 0; itr--)
					{
						if(mmaped_index[itr] == '\n')
						{
							itr++;
							break;
						}
					}
					if(strncmp(mmaped_index + itr, search_string,search_string_length))
						break;
					else
					{
						int64_t j;
						for(j = itr; mmaped_index[j] != '\n';j++)
						{
							foundstrings<<mmaped_index[j];
						}
						//foundstrings<<mmaped_index[j];
					}
					results.push_back(foundstrings.str());
					foundstrings.str("");
					i = itr;
				}
				break;

			}else if (cmp < 0)
			{
				lower = i + string_length;
				while(mmaped_index[lower] != '\n')
					lower++;
				lower++;
			}else if(cmp > 0)
			{
				upper = i - 1;
			}
			
		}
	}

}

void do_search(map<unsigned int,Metadata> &metadatas,vector<string> arguments)
{
	if(!arguments.size())
	{
		cerr<<"search: Missing argument, please enter an argument like \"foo * bar baz\" as a search string"<<endl;
		exit(-1);
	}
	if(arguments.size() > 1)
	{
		cerr<<"search: Too many arguments."<<endl;
		exit(-1);
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


	
	//Let's open the file,get its size, and then mmap it.
	int fd = open(index_filename.c_str(), O_RDWR);
	if(fd == -1)
	{
		cerr<<"Unable to open index" << index_filename<<endl;
		exit(-1);
	}
	struct stat fileinfo;
	if(stat(index_filename.c_str(),&fileinfo))
	{
		cerr<<"Unable to stat index" << index_filename<<endl;
		exit(-1);
	}
	

	char* mmaped_index = (char*) mmap(NULL,fileinfo.st_size,PROT_READ, MAP_PRIVATE,fd, 0);
	if(mmaped_index == (void*) -1)
	{
		cerr<<"Unable to mmap index " <<index_filename<<endl;
		exit(-1);
	}


	//We prepare out search tree.
	const char* c_search_string = tosearchfor.c_str();
	unsigned int search_string_length = strlen(c_search_string);
	if(search_string_length > fileinfo.st_size)
	{
		return;
	}

	map<string, off_t> search_tree;
	const char* ptr = mmaped_index;
       
	search_tree[string(ptr, search_string_length)] = 0;

	ptr = mmaped_index + fileinfo.st_size - 2;
	//Find the last string in the file.
	while((ptr > mmaped_index) && (*ptr != '\n'))
		ptr--;
	ptr++;
	//We add an artifical key that is one larger than the last key in the file to our search key.
	search_tree[string(ptr, search_string_length)] = fileinfo.st_size - 1;
	vector<string> results;

	single_search(mmaped_index, fileinfo.st_size,  search_tree,c_search_string,search_string_length,results);
	
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

	munmap(mmaped_index, fileinfo.st_size);
	return;		
}
