#include "get_top.h"

/* This function creates a table of offsets.
 * The code is relatively self-explanatory, it should however be noted that
 * even for the last index result[i+1] - result[i]  = the size of file i
 *
 */
pair<vector<string>,long long int>  getNGramAtAddress(off_t address, void* mmaped_file, off_t mmaped_file_size)
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

static off_t* offsets_table(string folder_name)
{
	off_t* result = (off_t*) malloc(sizeof(off_t) * 2);//NOTE: Update this if we ever deal with multiple input files again.
	result[0] = 0;
	struct stat cur;
	char* filename = (char*) malloc(strlen(folder_name.c_str()) + 1 +  3 + 4 + 1);
	strcpy(filename,folder_name.c_str());
	strcat(filename,"/");
	char* ptr = filename + strlen(filename);
	for(size_t i = 0; i< 1; i++)
	{
		sprintf(ptr, "%u.out",(unsigned int) i);
		if(stat(filename,&cur))
		{
			cerr<<"File "<<filename<<" does not exist"<<endl;
			exit(-1);
		}
		result[i + 1] = result[i] +  cur.st_size;
	}
	free(filename);
	return result;
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
	string filename = string("by");
	stringstream filename_stream(filename);
	filename_stream.seekp(0,std::ios::end);

	for(size_t i = 0; i< ngramsize;i++)
	{
		what_were_looking_for.push_back(i);
		filename_stream<<string("_");
		filename_stream<<i;
	}
	filename = filename_stream.str();

	if(find(relevant_metadata.inverted_indices.begin(), relevant_metadata.inverted_indices.end(),what_were_looking_for) == relevant_metadata.inverted_indices.end())
	{
		cerr<<"get_top: Did not find inverted index for the specified n"<<endl;
		exit(-1);
	}

	string folder_name = relevant_metadata.output_folder_name + "/" +  filename;
	off_t *offsets = offsets_table(folder_name);

	ifstream inverted_index_file((relevant_metadata.output_folder_name + string("/inverted_") + filename).c_str(),ios::in);
	if(!inverted_index_file)
	{
		cerr<<"get_top:"<<"Unable to open index file inverted_"<<filename<<endl;
		free(offsets);
		exit(-1);
	}
	string currentline;

	int fd = open((relevant_metadata.output_folder_name + "/"+filename+"/0.out").c_str(),O_RDONLY);
	if(fd < 0)
	{
		cerr<<"get_top: Could not open file"<<(relevant_metadata.output_folder_name + "/" + filename + "/0.out")<<endl;
		exit(-1);
	}
	off_t mmaped_file_size = lseek(fd, 0 , SEEK_END);

	void* mmaped_file = mmap(NULL,mmaped_file_size,PROT_READ,MAP_SHARED,fd,0);
	if(mmaped_file == (void*) -1)
	{
		cerr<<"get_top: Could not mmap file"<<(relevant_metadata.output_folder_name + "/" + filename + "/0.out")<<endl;
		exit(-1);
	}

	cerr<<"N-gram\tFrequency"<<endl;
	for(size_t i = 0; i< num_to_display && (getline(inverted_index_file,currentline) > 0);i++)
	{
		const char* to_work_with = currentline.c_str();
		char* endptr;
		long long frequency = strtoll(to_work_with,&endptr, 16);
		to_work_with = endptr;
		to_work_with++;
		off_t address = strtoll(to_work_with, &endptr, 16);
		auto this_ngram = getNGramAtAddress(address,mmaped_file,mmaped_file_size);
		bool first = true;
		for_each(this_ngram.first.begin(), this_ngram.first.end(), 
				[&first](string &cur)
				{
					if(!first)
						cout<<" ";
					cout<<cur;
					first = false;
				}
			);
		cout<<"\t"<<this_ngram.second<<endl;
		if(frequency != this_ngram.second)
		{
			cerr<<"get_top: Frequencies in inverted index by frequency do not match frequency of n-grams in the indexes by n-gram, probably one of the two indexes is out of date, exiting.."<<endl;
			munmap(mmaped_file,mmaped_file_size);
			close(fd);

		}
	}
	munmap(mmaped_file,mmaped_file_size);
	close(fd);
	free(offsets);

}

