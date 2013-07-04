#include "entropy_index_get_top.h"

static pair<vector<string>,long long int>  getNGramAtAddress(off_t address, void* mmaped_file, off_t mmaped_file_size)
{
	char* ptr = (char*)( mmaped_file) + address;
	char* ptr2 = ptr;
	vector<string> result;
	while(1)
	{
		if(strchr(" \t",*ptr2))
		{
			result.push_back(string(ptr,ptr2));
			if(*ptr2 == '\t')
			{
				char* endptr;
				long long int frequency = strtoll(ptr2,&endptr,10);
				return pair<vector<string>,long long int>(result,frequency);
			}
			ptr = ptr2 + 1;
		}
		ptr2++;
	}
}



void entropy_index_get_top(map<unsigned int,Metadata> &metadatas,vector<string> &arguments)
{
	//This function takes two arguments. The first is the n-gram number, and the second is the number of results to display.
	if(!((arguments.size() == 2) || (arguments.size() == 3)))
	{
		cerr<<"entropy_index_get_top: Please give two arguments, where the first is a string like \"= *\" second is the number of results to display."<<endl;
		exit(-1);
	}
	string search_string = arguments[0];

	unsigned int num_to_display = atoi(arguments[1].c_str());
	if(!num_to_display)
	{
		cerr<<"entropy_index_get_top: Please give a valid number for the n of n-gram that you want to search for as the second argument." <<endl;
		exit(-1);
	}
	unsigned int minimum_frequency_to_show = 1;
	if(arguments.size() == 3)
	{
		minimum_frequency_to_show = atoi(arguments[2].c_str());
		if(!minimum_frequency_to_show)
		{
			cerr<<"entropy_index_get_top: Please give a valid number for the minmum frequency of search strings with 0 entropy to show"<<endl;
			exit(-1);
		}
	}
	bool last_was_space = true;
	vector<unsigned int> known_words;
	vector<unsigned int> unknown_words;
	unsigned int current_position = 0;
	for(auto i = search_string.begin(); i != search_string.end();i++)
	{
		if(!last_was_space)
		{
			if(*i != ' ')
			{
				cerr<<"Argument was incorrectly formated"<<endl;
				exit(-1);
			}else
				last_was_space = true;

		}else
		{
			if(*i == ' ')
			{
				cerr<<"Do not use double spaces"<<endl;
				exit(-1);
			}else if (*i == '=')
			{
				known_words.push_back(current_position);
			}else
			{
				unknown_words.push_back(current_position);
			}
			last_was_space = false;
			current_position++;
		}
	}

	unsigned int ngramsize = known_words.size() + unknown_words.size();
	//We only build indexes which perfectly match for n-gram size, for now:
	auto relevant_metadata_itr = metadatas.find(ngramsize);
	if(relevant_metadata_itr == metadatas.end())
	{
		cerr<<"Could not find metadata for "<<ngramsize<<"-grams"<<endl;
		exit(-1);
	}
	Metadata &relevant_metadata = relevant_metadata_itr->second;
	unsigned int matches = 0;
	pair<unsigned int,vector<unsigned int> > search_index_to_use;
	for(auto i = relevant_metadata.entropy_indexes.begin(); i != relevant_metadata.entropy_indexes.end(); i++)
	{
		if(i->first != known_words.size())
			continue;
		unsigned int current_matches = 0;	
		for(auto j = i->second.begin(); j != i->second.end(); j++)
		{
			if(find(known_words.begin(), known_words.end(),*j) != known_words.end())
				current_matches++;
			else
				break;
		}
		if(current_matches > matches)
		{
			matches = current_matches;
			search_index_to_use = *i;
		}
	}
	if(matches != known_words.size())
	{
		cerr<<"Could not find a any entropy indexes for this search string.\n"<<endl;
		exit(-1);
	}

	string index_filename("");
	stringstream index_filename_stream("");
	index_filename_stream.seekp(0,std::ios::end);

	for(size_t i = 0; i< ngramsize;i++)
	{
		index_filename_stream<<"_";
		index_filename_stream<<search_index_to_use.second[i];
	}
	index_filename = relevant_metadata.output_folder_name +"./by" + index_filename_stream.str() + "/0.out";
	stringstream number("");
	number<<known_words.size();
	string entropy_index_filename = relevant_metadata.output_folder_name + "/entropy_" + number.str()+"_index"+index_filename_stream.str();

	FILE* entropy_index_file = fopen(entropy_index_filename.c_str(),"r");
	if(!entropy_index_file)
	{
		cerr<<"Could not open file "<<entropy_index_filename<<endl;
		exit(-1);

	}
	string currentline;
	cerr<<"N-gram\tEntropy(in bits)"<<endl;
	int index_fd = open(index_filename.c_str(),  O_RDONLY);
	if(index_fd < 0)
	{
		cerr<<"Could not open file "<<index_filename<<endl;
		exit(-1);
	}
	off_t index_file_size = lseek(index_fd,0,SEEK_END);

	void* mmaped_index_file = mmap(NULL,index_file_size, PROT_READ,MAP_SHARED,index_fd,0);
	if(mmaped_index_file == (void*) -1)
	{
		cerr<<"Could not mmap file"<<index_filename;
	}
	posix_madvise(mmaped_index_file,index_file_size, POSIX_MADV_RANDOM);

	for(int i = 0; i<(int) num_to_display;i++)
	{
		float current_entropy;
		uint64_t current_offset;

		fread(&current_entropy,sizeof(current_entropy),1,entropy_index_file);
		size_t read = fread(&current_offset,sizeof(current_offset),1,entropy_index_file);
		if(!read)
			break;

		pair<vector<string>,long long int> ngram = getNGramAtAddress(current_offset,mmaped_index_file,index_file_size);
		if((current_entropy == 0.0) && (ngram.second < minimum_frequency_to_show))
		{
			i--;
			continue;
		}else
		{
			for(size_t i = 0; i < unknown_words.size(); i++)
			{
				ngram.first[ngram.first.size() - 1 - i] = "*";
			}
	
			for(size_t i = 0; i < ngramsize; i++)
			{
				auto position = find(search_index_to_use.second.begin(),search_index_to_use.second.end(), i);
				cout<<ngram.first[position - search_index_to_use.second.begin()];
				if(i != (ngramsize-1))
					cout<<" ";
			}
			cout<<"\t"<<current_entropy<<endl;
		}
	}
	munmap(mmaped_index_file,index_file_size);
	close(index_fd);

}

