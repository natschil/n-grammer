#include "search.h"

using namespace std;

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
	size_t ngramsize = 0;
	vector<unsigned int> known;
	vector<string> strings;

	while(getline(argumentstream,cur,' '))
	{
		strings.push_back(cur);						
		if(cur != "*")
		{
			known.push_back(ngramsize);
		}
		ngramsize++;
	}

	if(metadatas.find(ngramsize) == metadatas.end())
	{
		cerr<<"search: Unable to find metadata for ngrams of size "<<ngramsize<<endl;
		exit(-1);
	}

	Metadata &relevant_metadata = metadatas[ngramsize];

	//At this point we try to find the most suitable combination
	vector<unsigned int> most_suitable_combination;	
	size_t matches = 0;
	for(set<vector<unsigned int> >::iterator itr = relevant_metadata.indices.begin(); itr != relevant_metadata.indices.end();itr++)
	{
		size_t counter = 0;
		for(vector<unsigned int>::const_iterator j = (*itr).begin(); j != (*itr).end(); j++)
		{
			if(find(known.begin(),known.end(),*j) != known.end())
				counter++;
			else break;
		}
		if(counter > matches)
		{
			most_suitable_combination = *itr;
			matches = counter;
		}
	}
	if(!matches)
	{
		cerr<<"search: No indexes found that could do this type of search."<<endl;
		exit(-1);
	}
	cerr<<"search: Searching with "<<matches<<" matches using index by";
	for(size_t i = 0; i < most_suitable_combination.size(); i++)
	{
		cerr<<"_";
		cerr<<most_suitable_combination[i];
	}
	cerr<<endl;
	string index_filename = relevant_metadata.folder_name + "/by";
	stringstream index_filename_stream(index_filename);
	index_filename_stream.seekp(0, std::ios::end);

	for(size_t i = 0; i< most_suitable_combination.size(); i++)
	{
		index_filename_stream<<"_";
		index_filename_stream<<most_suitable_combination[i];
	}
	index_filename_stream<<"/";
	//index_filename_stream<<(int)(uint8_t) (char) strings[most_suitable_combination[0]].c_str()[0]<<".out";
	index_filename_stream<<"0.out";
	index_filename = index_filename_stream.str();
	
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

	//At this point we do a binary search on the file...
	
	string tosearchfor = string("");
	for(size_t i = 0; i< matches; i++)
	{
		if(i != 0)
			tosearchfor += " ";
		tosearchfor += strings[most_suitable_combination[i]];
	}
	if(matches == most_suitable_combination.size())
	{
		tosearchfor += "\t";
	}else
		tosearchfor += " ";

	
	int64_t upper = fileinfo.st_size -1;
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

			int cmp = strncmp(mmaped_index + i,tosearchfor.c_str(),strlen(tosearchfor.c_str()));
			if(cmp == 0)
			{
				int64_t  itr;
				for(itr = i; itr<fileinfo.st_size ;itr++)
				{
					foundstrings<<mmaped_index[itr];
					if((mmaped_index[itr] == '\n') && (itr < (fileinfo.st_size -1)))
					{
						itr++;
						if(strncmp(mmaped_index +itr, tosearchfor.c_str(),strlen(tosearchfor.c_str())))
						{
							break;
						}
						itr--; 
					}
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
					if(strncmp(mmaped_index + itr, tosearchfor.c_str(),strlen(tosearchfor.c_str())))
						break;
					else
					{
						int64_t j;
						for(j = itr; mmaped_index[j] != '\n';j++)
						{
							foundstrings<<mmaped_index[j];
						}
						foundstrings<<mmaped_index[j];
					}
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
		//Get the string at this point
	}

	string current_ngram;
	while(getline(foundstrings,current_ngram))
	{
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
		for(size_t i=0; i<most_suitable_combination.size(); i++)
		{
			if(i)
				cout<<" ";
			for(size_t j = 0; j < most_suitable_combination.size(); j++)
			{
				if(most_suitable_combination[j] == i)
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
