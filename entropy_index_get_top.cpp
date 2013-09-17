#include "entropy_index_get_top.h"

void entropy_index_get_top(map<unsigned int,Metadata> &metadatas,vector<string> &arguments)
{
    //First we open the arguments and check them
    
	//This function takes two arguments. The first is the n-gram number, and the second is the number of results to display.
	if(!((arguments.size() == 2) || (arguments.size() == 3)))
	{
		cerr<<"entropy_index_get_top: Please give two arguments, where the first is a string like \"= *\"and the second is the number of results to display."<<endl;
		exit(-1);
	}
	string search_string = arguments[0];
	if(strspn(search_string.c_str()," *=") != search_string.length())
	{
		cerr<<"Please give an argument like \"* = * = ="<<endl;
	}

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
	vector<string> tokenized_search_string = split_ignoring_empty(search_string,' ');
	vector<unsigned int> known_words;
	vector<unsigned int> unknown_words;

	for(size_t i = 0; i<tokenized_search_string.size(); i++)
	{
		string cur = tokenized_search_string[i];
		if(cur.length() != 1)
		{
			cerr<<"entropy_index_get_top: Invalid search string format descriptor: "<<cur<<endl;
			exit(-1);
		}
		switch(cur[0])
		{
			case '*':
				unknown_words.push_back(i);
			break;
			case '=':
				known_words.push_back(i);
			break;
		}
	}

    //We find the relevant indices for this:
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

    //We now open the relevant files

	string index_specifier = join(functional_map(search_index_to_use.second,unsigned_int_to_string),"_");
	string frequency_index_filename = relevant_metadata.output_folder_name + "/by_" +  index_specifier + "/0.out";
	string entropy_index_filename = relevant_metadata.output_folder_name +
	      			 	"/entropy_" + to_string(known_words.size()) + "_index_" + index_specifier;

	FILE* entropy_index_file = fopen(entropy_index_filename.c_str(),"r");
	if(!entropy_index_file)
	{
		cerr<<"Could not open file "<<entropy_index_filename<<endl;
		exit(-1);
	}

	size_t header_line_size = 1024;
	char* header_line = (char*)malloc(header_line_size);
	if(getline(&header_line,&header_line_size,entropy_index_file) <= 0)
	{
		cerr<<"Could not read enough bytes from file "<<entropy_index_filename<<endl;
		exit(-1);
	}
	if(strcmp(header_line,"Entropy Index File in Binary Format\n"))
	{
		cerr<<"Entropy index file "<<entropy_index_filename<<" has incorrect header part one."<<endl;
		exit(-1);
	}
	if(getline(&header_line,&header_line_size,entropy_index_file) <= 0)
	{
		cerr<<"Could not read enough bytes from file "<<entropy_index_filename<<endl;
		exit(-1);
	}
	if(strcmp(header_line,"---Begin Binary Reference Info---\n"))
	{
		cerr<<"Entropy index file "<<entropy_index_filename<<" has incorrect header part two."<<endl;
		exit(-1);
	}
	float float_to_test;
	uint64_t uint_to_test;
	if(fread(&float_to_test,sizeof(float_to_test),1,entropy_index_file) != 1)
	{
		cerr<<"Could not read reference float from entropy index file "<<entropy_index_filename<<endl;
		exit(-1);
	}
	if(fread(&uint_to_test,sizeof(uint_to_test),1,entropy_index_file) != 1)
	{
		cerr<<"Could not read reference uint from entropy index file "<<entropy_index_filename<<endl;
		exit(-1);
	}
	if((float_to_test != 123.125) || (uint_to_test != 1234567890))
	{
		cerr<<"It seems like the entropy indexes were not generated on this machine, and this machine uses a different binary format"<<endl;
		exit(-1);
	}
	if(getc(entropy_index_file) != '\n')
	{
		cerr<<"Expected newline in entropy index file, did not find it, exiting"<<endl;
		exit(-1);
	}
	if(getline(&header_line,&header_line_size,entropy_index_file) <= 0)
	{
		cerr<<"Could not read enough bytes from file "<<entropy_index_filename<<endl;
		exit(-1);
	}
	if(strcmp(header_line,"---End Binary Reference Info---\n"))
	{
		cerr<<"Entropy index file "<<entropy_index_filename<<" has incorrect header part three."<<endl;
		exit(-1);
	}

	if(getline(&header_line,&header_line_size,entropy_index_file) <= 0)
	{
		cerr<<"Could not read enough bytes from file "<<entropy_index_filename<<endl;
		exit(-1);
	}
	if(strcmp(header_line,"---Actual Index is in Binary Form Below---\n"))
	{
		cerr<<"Entropy index file "<<entropy_index_filename<<" has incorrect header part four."<<endl;
		exit(-1);
	}

	string currentline;
	cerr<<"N-gram\tEntropy(in bits)"<<endl;
	int frequency_index_fd = open(frequency_index_filename.c_str(),  O_RDONLY);
	if(frequency_index_fd < 0)
	{
		cerr<<"Could not open file "<<frequency_index_filename<<endl;
		exit(-1);
	}
	off_t frequency_index_file_size = lseek(frequency_index_fd,0,SEEK_END);

	void* mmaped_frequency_index = mmap(NULL,frequency_index_file_size, PROT_READ,MAP_SHARED,frequency_index_fd,0);
	if(mmaped_frequency_index == (void*) -1)
	{
		cerr<<"Could not mmap file"<<frequency_index_filename;
		exit(-1);
	}
	posix_madvise(mmaped_frequency_index,frequency_index_file_size, POSIX_MADV_RANDOM);

	for(int i = 0; i<(int) num_to_display;i++)
	{
		float current_entropy;
		uint64_t current_offset;

		if(fread(&current_entropy,sizeof(current_entropy),1,entropy_index_file) != 1)
		{
			cerr<<"Reached end of file, exiting (but with zero exit status)"<<endl;
			break;
		}
		if(fread(&current_offset,sizeof(current_offset),1,entropy_index_file) != 1)
		{
			cerr<<"Reached premature eof, exiting (with non-zero exit status)"<<endl;	
			exit(-1);
		}

		pair<vector<string>,long long int> ngram = getNGramAtAddress(current_offset,mmaped_frequency_index,frequency_index_file_size);
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
					cout<<'\t';
			}
			cout<<"\t"<<current_entropy<<endl;
		}
	}
	munmap(mmaped_frequency_index,frequency_index_file_size);
	close(frequency_index_fd);
	return;
}

