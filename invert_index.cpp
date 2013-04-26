#include "invert_index.h"



void invert_index(map<unsigned int,Metadata> &metadatas,vector<string> &arguments)
{
	if(!arguments.size())
	{
		cerr<<"invert_index: Missing argument, please enter an argument like 0_1_2_3 to specify which index to invert"<<endl;
		exit(-1);
	}
	if(arguments.size() > 1)
	{
		cerr<<"invert_index: Too many arguments."<<endl;
		exit(-1);
	}

	size_t ngram_size = (arguments[0].length() + 1)/2;
	if(metadatas.find(ngram_size) == metadatas.end())
	{
		cerr<<"invert_index: Either the argument is invalid, or there doesn't exist a "<<ngram_size<<"_grams.metadata file"<<endl;
		exit(-1);
	}

	Metadata &relevant_metadata = metadatas[ngram_size];
	string foldername = relevant_metadata.folder_name + string("/") + string("by_") + arguments[0];
	string out_filename = relevant_metadata.folder_name +string("/"	)+  string("inverted_") + arguments[0];

	DIR* argument_folder = opendir(foldername.c_str());
	if(!argument_folder)
	{
		cerr<<"invert_index: Unable to open folder"<<arguments[0]<<endl;
		exit(-1);
	}
	closedir(argument_folder);
	long long int file_starts_at[256];
	for(size_t i = 0; i<256;i++)
	{
		struct stat current_file;
		char buf[3];
		snprintf(buf,3,"%lu",i);
		string filename = foldername + string("/")  + string(buf) + string(".out");
		if(stat(filename.c_str(), &current_file))
		{
			cerr<<"invert_index: Unable to stat "<<filename<<endl;
			exit(-1);
		}
		file_starts_at[i] = (i == 0) ? current_file.st_size: file_starts_at[i -1] + current_file.st_size;
	}

	long long *table = (long long*) calloc(relevant_metadata.max_frequency + 1,sizeof(*table));

	//The first pass of a distribution sort.
	char* filename_buf = (char*) malloc(foldername.length() + strlen("/") + strlen("256") + strlen(".out") + 1);
	strcpy(filename_buf, foldername.c_str());
	strcat(filename_buf,"/");

	char* ptr = filename_buf + strlen(filename_buf);
	
	char* string_buf = (char*) malloc(1024); //This is more than enough.
	size_t string_buf_size = 1024;
	size_t output_file_size = 0;
	for(size_t i = 0; i< 256; i++)
	{
		//We use C here instead of C++ for maximum efficientcy
		sprintf(ptr,"%lu.out",i);
		FILE* currentfile = fopen(filename_buf,"r");
		if(!currentfile)
		{
			cerr<<"Unable to open "<<filename_buf<<" for reading"<<endl;
			exit(-1);
		}
		ssize_t num_read;
		while((num_read = getline(&string_buf,&string_buf_size,currentfile)) > 0)
		{
			const char* endptr = string_buf + num_read - 1; 
			endptr--; //Get rid of ending newline...
			while(isdigit(*endptr))
				endptr--;
			char* errptr;
			long long number = strtoll(endptr,&errptr,10);
			if((*errptr) != '\n'|| !number)
			{
				fprintf(stderr,"invert_index: File to read %lu has lines but no lines ending numbers then newlines\n",i);
			}
			table[number]++;
			output_file_size += (4 + 1 + 4 + 1); //4 bits per hex-digit
		}
		fclose(currentfile);
	}
	//We sift up:
	for(size_t i = 1; i<=relevant_metadata.max_frequency;i++)
	{
		table[i] += table[i -1];
	}

	int fd = open(out_filename.c_str(),O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if(fd == -1)
	{
		cerr<<"invert_index: Unable to open file "<<out_filename.c_str()<<endl;
		exit(-1);
	}
	lseek(fd, output_file_size - 1 ,SEEK_SET);
	if(write(fd,"",1) != 1)
	{
		cerr<<"invert_index: Unable to write to to file "<<out_filename.c_str()<<endl;
		exit(-1);
	}

	void* outfile_map = mmap(NULL,output_file_size,PROT_WRITE | PROT_READ,MAP_SHARED,fd,0);
	if(outfile_map == MAP_FAILED)
	{
		cerr<<"invert_index: Unable to mmap file"<<endl;
		exit(-1);
	}

	for(size_t i = 0; i< 256; i++)
	{
		long long current_offset = file_starts_at[i];
		//We use C here instead of C++ for maximum efficientcy
		sprintf(ptr,"%lu.out",i);
		FILE* currentfile = fopen(filename_buf,"r");
		ssize_t num_read;
		while((num_read = getline(&string_buf,&string_buf_size,currentfile)) > 0)
		{
			const char* endptr = string_buf + num_read -1;
			endptr--; //Get rid of ending newline...
			while(isdigit(*endptr))
				endptr--;
			char* errptr;
			long long number = strtoll(endptr,&errptr,10);
			long long int pos = --table[number]; 

			char* write_pos = (char*) outfile_map + pos*(4+1+4+1);
			snprintf(write_pos,(4+ 1+4+1), "%04llX\t%04llX",number, current_offset);
			write_pos[4 + 1 +4 +1 -1] = '\n';
			current_offset += num_read;
		}
		fclose(currentfile);
	}
	munmap(outfile_map,output_file_size);
	return;

}

