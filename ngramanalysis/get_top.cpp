#include "get_top.h"

/*
 * This function gets an n-gram at the specified offset from an mmaped file of size mmaped_file_size.
 */

pair<vector<string>,long long int>  getNGramAtAddress(off_t address, void* mmaped_file, off_t mmaped_file_size)
{
	if(address >= mmaped_file_size)
	{
		cerr<<"Trying to get ngram from non-existant point in n-gram frequency index"<<endl;
		exit(-1);
	}
	char* ptr = (char*)( mmaped_file) + address;
	char* ptr2 = ptr;
	vector<string> result;
	while(1)
	{
		if(*ptr2 == '\t')
		{
			result.push_back(string(ptr,ptr2));
			//This relies on the fact that digit characters are not part of n-grams.
			if(isdigit(*(ptr2+1)))
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

void get_top(map<unsigned int,Metadata> &metadatas,vector<string> &arguments)
{
	//This function takes two arguments. The first is the n-gram number, and the second is the number of results to display.
	if(arguments.size() != 2)
	{
		cerr<<"search_inverted_index: Please give two arguments, where the first is the number of words in the n-gram and the second is the number of results to display."<<endl;
		exit(-1);
	}
	unsigned int ngramsize = atoi(arguments[0].c_str());
	if(!ngramsize)
	{
		cerr<<"search_inverted_index: Please give a valid non-zero number for the n-gram size as the first argument."<<endl;
		exit(-1);
	}

	unsigned int num_to_display = atoi(arguments[1].c_str());
	if(!num_to_display)
	{
		cerr<<"search_inverted_index: Please give a valid number for the n of n-gram that you want to search for as the second argument." <<endl;
		exit(-1);
	}

	auto relevant_metadata_itr = metadatas.find(ngramsize);
	if(relevant_metadata_itr == metadatas.end())
	{
		cerr<<"search_inverted_index: No metadata for "<<ngramsize<<"-grams found"<<endl;
		exit(-1);
	}

	Metadata &relevant_metadata = relevant_metadata_itr->second;
	vector<unsigned int> what_were_looking_for;
	for(size_t i= 0; i< ngramsize; i++)
		what_were_looking_for.push_back(i);

	function<string(unsigned int)> unsigned_to_string = [](unsigned int in) -> string { return to_string(in);};
	string index_specification = join(functional_map(what_were_looking_for,unsigned_to_string),"_");

	string frequency_index_filename = relevant_metadata.output_folder_name + "/by_" + index_specification + "/0.out";
	string inverted_frequency_index_filename = relevant_metadata.output_folder_name + "/inverted_by_" + index_specification;

	if(find(relevant_metadata.inverted_indices.begin(), relevant_metadata.inverted_indices.end(),what_were_looking_for) == relevant_metadata.inverted_indices.end())
	{
		cerr<<"get_top: Did not find inverted index for the specified n"<<endl;
		exit(-1);
	}

	ifstream inverted_frequency_index_file(inverted_frequency_index_filename.c_str());
	if(!inverted_frequency_index_file)
	{
		cerr<<"get_top:"<<"Unable to open index file "<<inverted_frequency_index_filename<<endl;
		exit(-1);
	}

	int frequency_index_fd = open(frequency_index_filename.c_str(),O_RDONLY);
	if(frequency_index_fd < 0)
	{
		cerr<<"get_top: Could not open file"<<frequency_index_filename<<endl;
		exit(-1);
	}
	off_t frequency_index_file_size = lseek(frequency_index_fd, 0 , SEEK_END);

	void* mmaped_frequency_index = mmap(NULL,frequency_index_file_size,PROT_READ,MAP_SHARED,frequency_index_fd,0);
	if(mmaped_frequency_index == (void*) -1)
	{
		cerr<<"get_top: Could not mmap file"<<frequency_index_filename<<endl;
		exit(-1);
	}

	cerr<<"N-gram\tFrequency"<<endl;
	string currentline;
	//TODO: Possible write it out if we reach EOF too soon.
	for(size_t i = 0; i< num_to_display && getline(inverted_frequency_index_file,currentline);i++)
	{
		const char* to_work_with = currentline.c_str();
		char* endptr;
		long long frequency = strtoll(to_work_with,&endptr, 16);
		to_work_with = endptr;
		to_work_with++;//Get rid of the "space" character in between.
		off_t address = strtoll(to_work_with, &endptr, 16);
		auto this_ngram = getNGramAtAddress(address,mmaped_frequency_index,frequency_index_file_size);

		cout<<join(this_ngram.first,"\t")<<"\t"<<this_ngram.second<<endl;
		if(frequency != this_ngram.second)
		{
			cerr<<"get_top: Frequencies in inverted index by frequency do not match frequency of n-grams in the indexes by n-gram, probably one of the two indexes is out of date, exiting.."<<endl;
			munmap(mmaped_frequency_index,frequency_index_file_size);
			close(frequency_index_fd);

		}
	}
	munmap(mmaped_frequency_index,frequency_index_file_size);
	close(frequency_index_fd);

}

