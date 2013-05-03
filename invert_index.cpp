#include "invert_index.h"

static inline unsigned int integer_pow_10(unsigned int num)
{
	int result = 1;
	while(num--)
		result *= 10;
	return result;
}

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

	size_t ngram_size = (std::count(arguments[0].begin(), arguments[0].end(), '_') + 1);
	if(metadatas.find(ngram_size) == metadatas.end())
	{
		cerr<<"invert_index: Either the argument is invalid, or there doesn't exist a "<<ngram_size<<"_grams.metadata file"<<endl;
		exit(-1);
	}

	Metadata &relevant_metadata = metadatas[ngram_size];
	string foldername = relevant_metadata.folder_name + string("/") + string("by_") + arguments[0];
	string out_filename = relevant_metadata.folder_name +string("/"	)+  string("inverted_by_") + arguments[0];

	DIR* argument_folder = opendir(foldername.c_str());
	if(!argument_folder)
	{
		cerr<<"invert_index: Unable to open folder"<<foldername<<endl;
		exit(-1);
	}
	closedir(argument_folder);
	long long int file_starts_at[256];
	file_starts_at[0] = 0;
	for(size_t i = 0; i<1;i++)
	{
		struct stat current_file;
		char buf[4];
		snprintf(buf,4,"%u",(unsigned int)i);
		string filename = foldername + string("/")  + string(buf) + string(".out");
		if(stat(filename.c_str(), &current_file))
		{
			cerr<<"invert_index: Unable to stat "<<filename<<endl;
			exit(-1);
		}
		file_starts_at[i + 1] = current_file.st_size + file_starts_at[i];
	}

	if(!relevant_metadata.max_frequency)
	{
		long long int max_freq = 0;
		char buf[4];
		char* other_buf =(char*) malloc(1024);
		size_t other_buf_size = 1024;
		for(size_t i = 0; i< 1; i++)
		{
			snprintf(buf,4,"%u",(unsigned int)i);
			string filename = foldername + string("/") + string(buf) + string(".out");
			FILE* cur = fopen(filename.c_str(),"r");
			if(!cur)
			{
				cerr<<"Could not open file "<<filename<<endl;
				exit(-1);
			}
			ssize_t read;
			while((read = getline(&other_buf, &other_buf_size,cur)) > 0)
			{
				const char* ptr = other_buf + read - 1 -1;
				long long number = 0;
				int index = 0;
				while(1)
				{
					switch(*ptr--)
					{
					case '0':;
					break;
					case '1':number += 1*integer_pow_10(index);
					break;
					case '2': number += 2*integer_pow_10(index);
					break;
					case '3':number += 3*integer_pow_10(index);
					break;
					case '4':number += 4*integer_pow_10(index);
					break;
					case '5':number += 5*integer_pow_10(index);
					break;
					case '6':number += 6*integer_pow_10(index);
					break;
					case '7':number += 7*integer_pow_10(index);
					break;
					case '8':number += 8*integer_pow_10(index);
					break;
					case '9':number += 9*integer_pow_10(index);
					break;
					default: goto outofloop;

					}
					index++;
				}
outofloop:
				if(number > max_freq)
					max_freq = number;
			}
			fclose(cur);

		}
		free(other_buf);
		relevant_metadata.max_frequency =max_freq;
	}

	long long int *table = (long long int*) calloc(relevant_metadata.max_frequency + 1,sizeof(*table));

	//The first pass of a distribution sort.
	char* filename_buf = (char*) malloc(foldername.length() + strlen("/") + strlen("256") + strlen(".out") + 1);
	strcpy(filename_buf, foldername.c_str());
	strcat(filename_buf,"/");

	char* ptr = filename_buf + strlen(filename_buf);
	
	char* string_buf = (char*) malloc(1024); //This is more than enough.
	size_t string_buf_size = 1024;
	size_t output_file_size = 0;
	for(size_t i = 0; i< 1; i++)
	{
		//We use C here instead of C++ for maximum efficiency
		sprintf(ptr,"%lu.out",i);
		FILE* currentfile = fopen(filename_buf,"r");
		if(!currentfile)
		{
			free(string_buf);
			free(filename_buf);
			free(table);
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
				free(string_buf);
				free(filename_buf);
				free(table);
				exit(-1);
			}
			table[number]++;
			output_file_size += (16 + 1 + 16 + 1); //16 characters per 64 bit hex number
		}
		fclose(currentfile);
	}
	//We sift up:
	for(size_t i = relevant_metadata.max_frequency; i>1;i--)
	{
		table[i -1] += table[i];
	}

	int fd = open(out_filename.c_str(),O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if(fd == -1)
	{
		free(string_buf);
		free(filename_buf);
		free(table);
		cerr<<"invert_index: Unable to open file "<<out_filename.c_str()<<endl;
		exit(-1);
	}

	ftruncate(fd,output_file_size);

	void* outfile_map = mmap(NULL,output_file_size,PROT_WRITE | PROT_READ,MAP_SHARED,fd,0);
	if(outfile_map == MAP_FAILED)
	{
		free(string_buf);
		free(filename_buf);
		free(table);
		cerr<<"invert_index: Unable to mmap file "<<out_filename.c_str()<<endl;
		exit(-1);
	}

	for(size_t i = 0; i< 1; i++)
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

			char* write_pos = (char*) outfile_map + pos*(16+1+16+1);
			snprintf(write_pos,(16+ 1+16+1), "%16llX\t%16llX",number, current_offset);
			write_pos[16 + 1 +16 +1 -1] = '\n';
			current_offset += num_read;
		}
		fclose(currentfile);
	}
	munmap(outfile_map,output_file_size);
	vector<unsigned int> index_name_as_vector;
	stringstream forsplitting(arguments[0]);
	while(!forsplitting.eof())
	{
		unsigned int cur;
		forsplitting>>cur;
		forsplitting.get();
		index_name_as_vector.push_back(cur);
	}
	
	relevant_metadata.inverted_indices.insert(index_name_as_vector);
	relevant_metadata.write();
	free(filename_buf);
	free(table);
	free(string_buf);
	return;

}

