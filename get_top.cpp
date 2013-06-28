#include "get_top.h"

/* This function creates a table of offsets.
 * The code is relatively self-explanatory, it should however be noted that
 * even for the last index result[i+1] - result[i]  = the size of file i
 *
 */
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

static void writeOutAddress(off_t address, off_t* offsets, string foldername,long long should_be)
{
	int i;

	for( i= 1;(i > 0) &&( offsets[i] > address); i--)
		;

	
	off_t real_offset = address - offsets[i];

	char* filename = (char*) malloc(strlen(foldername.c_str()) + 1 + 3 +4 + 1);
	strcpy(filename, foldername.c_str());
	strcat(filename,"/");
	sprintf(filename + strlen(filename), "%u.out", (unsigned int)i);

	int fd = open(filename,  O_RDONLY);
	if(fd < 0)
	{
		cerr<<"Could not open file "<<filename<<endl;
		free(filename);
		exit(-1);
	}
	free(filename);

	void* mmaped_file = mmap(NULL,offsets[i+1] - offsets[i], PROT_READ,MAP_SHARED,fd,0);
	if(mmaped_file == (void*) -1)
	{
		cerr<<"Could not mmap file"<<endl;
		exit(-1);
	}
	madvise(mmaped_file, offsets[i+1] - offsets[i],MADV_RANDOM);
	char* ptr = (char*)( mmaped_file) + real_offset;
	while(*ptr != '\t')
	{
		cout<<*(ptr++);
	}
	long long actual = strtoll(ptr,NULL,10);
	if(actual != should_be)
	{
		/*
		cerr<<"Values in index do not correspond with those in n-gram database, exiting"<<endl;
		munmap(mmaped_file,offsets[i+1] - offsets[i]);
		exit(-1);
		*/
	}

	munmap(mmaped_file,offsets[i+1] -offsets[i]);
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
		cerr<<"search_inverted_index: Did not find inverted index for the specified n"<<endl;
		exit(-1);
	}

	string folder_name = relevant_metadata.output_folder_name + "/" +  filename;
	off_t *offsets = offsets_table(folder_name);

	ifstream inverted_index_file((relevant_metadata.output_folder_name + string("/inverted_") + filename).c_str(),ios::in);
	if(!inverted_index_file)
	{
		cerr<<"search_inverted_index:"<<"Unable to open index file inverted_"<<filename<<endl;
		free(offsets);
		exit(-1);
	}
	string currentline;
	cerr<<"N-gram\tFrequency"<<endl;
	for(size_t i = 0; i< num_to_display && getline(inverted_index_file,currentline);i++)
	{
		const char* to_work_with = currentline.c_str();
		char* endptr;
		long long frequency = strtoll(to_work_with,&endptr, 16);
		to_work_with = endptr;
		to_work_with++;
		off_t address = strtoll(to_work_with, &endptr, 16);
		writeOutAddress(address,offsets,folder_name,frequency);
		cout<<"\t"<<frequency<<endl;
	}
	free(offsets);

}

