#include "entropy_index_get_top.h"

static vector<string> getNGramAtAddress(off_t address, string filename)
{

	int fd = open(filename.c_str(),  O_RDONLY);
	if(fd < 0)
	{
		cerr<<"Could not open file "<<filename<<endl;
		exit(-1);
	}
	off_t file_size = lseek(fd,0,SEEK_END);

	void* mmaped_file = mmap(NULL,file_size, PROT_READ,MAP_SHARED,fd,0);
	if(mmaped_file == (void*) -1)
	{
		cerr<<"Could not mmap file"<<endl;
		exit(-1);
	}
	madvise(mmaped_file, file_size,MADV_RANDOM);
	char* ptr = (char*)( mmaped_file) + address;
	char* ptr2 = ptr;
	vector<string> result;
	while(1)
	{
		if(strchr(" \t",*ptr2))
		{
			result.push_back(string(ptr,ptr2));
			if(*ptr == '\t')
				break;
			ptr = ptr2 + 1;
		}
		ptr2++;
	}
	munmap(mmaped_file,file_size);
	return result;
}



void entropy_index_get_top(map<unsigned int,Metadata> &metadatas,vector<string> &arguments)
{
	//This function takes two arguments. The first is the n-gram number, and the second is the number of results to display.
	if(arguments.size() != 2)
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
	bool last_was_space = true;
	vector<unsigned int> known_words;
	vector<unsigned int> unknown_words;
	unsigned int current_position = 0;
	for(auto i = search_string.begin(); i != search_string.end();i++,current_position++)
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
	stringstream index_filename_stream(index_filename);
	index_filename_stream.seekp(0,std::ios::end);

	for(size_t i = 0; i< ngramsize;i++)
	{
		index_filename_stream<<"_";
		index_filename_stream<<search_index_to_use.second[i];
	}
	index_filename = relevant_metadata.output_folder_name +"./by" + index_filename_stream.str() + "/0.out";
	stringstream number("");
	number<<known_words.size();
	string entropy_index_filename = relevant_metadata.output_folder_name + "./entropy_" + number.str()+"_index"+index_filename_stream.str();

	ifstream entropy_index_file(entropy_index_filename.c_str(),ios::in);
	if(!entropy_index_file)
	{
		cerr<<"Could not open file "<<entropy_index_filename<<endl;
		exit(-1);

	}
	string currentline;
	cerr<<"N-gram\tEntropy(in bits)"<<endl;
	for(size_t i = 0; i< num_to_display && getline(entropy_index_file,currentline);i++)
	{
		const char* to_work_with = currentline.c_str();
		char* endptr;
		unsigned int entropy = strtoul(to_work_with,&endptr, 16);
		float* entropy_f = reinterpret_cast<float*> (&entropy);

		to_work_with = endptr;
		to_work_with++;
		off_t address = strtoll(to_work_with, &endptr, 16);
		vector<string> ngram = getNGramAtAddress(address,index_filename);
		for(size_t i = 0; i < known_words.size(); i++)
		{
			cout<<ngram[known_words[i]]<<" ";
		}
		for(size_t i = 0; i < unknown_words.size(); i++)
		{
			cout<<"*";
			if((i+1) != unknown_words.size())
				cout<<" ";

		}
		cout<<"\t"<<*entropy_f<<endl;
	}

}

