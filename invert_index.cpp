#include "invert_index.h"

static inline unsigned int integer_pow_10(unsigned int num)
{
	int result = 1;
	while(num--)
		result *= 10;
	return result;
}

static void get_max_frequency(Metadata &relevant_metadata, string &foldername)
{
	long long int max_freq = 0;

	string filename = foldername + string("/0.out");
	FILE* index_file = fopen(filename.c_str(),"r");
	if(!index_file)
	{
		cerr<<"Could not open file "<<filename<<endl;
		exit(-1);
	}

	ssize_t read;
	char* line_buf =(char*) malloc(1024);
	size_t line_buf_size = 1024;
	while((read = getline(&line_buf, &line_buf_size,index_file)) > 0)
	{
		//Set ptr to point to the newline after the number.
		const char* ptr = line_buf + read - 1;
		long long int number = 0;
		int current_power = 0;
		while(--ptr >= line_buf)
		{
			switch(*ptr)
			{
			case '0':;
			break;
			case '1':number += 1*integer_pow_10(current_power);
			break;
			case '2': number += 2*integer_pow_10(current_power);
			break;
			case '3':number += 3*integer_pow_10(current_power);
			break;
			case '4':number += 4*integer_pow_10(current_power);
			break;
			case '5':number += 5*integer_pow_10(current_power);
			break;
			case '6':number += 6*integer_pow_10(current_power);
			break;
			case '7':number += 7*integer_pow_10(current_power);
			break;
			case '8':number += 8*integer_pow_10(current_power);
			break;
			case '9':number += 9*integer_pow_10(current_power);
			break;
			default: goto outofloop;
			}
			current_power++;
		}
		outofloop:
		if(number > max_freq)
			max_freq = number;
	}
	fclose(index_file);
	free(line_buf);
	relevant_metadata.max_frequency = max_freq;
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

	//Something like 0_1_2_3 as an argument has three '_' characters and therefore refers to a 4-gram index.
	size_t ngram_size = (std::count(arguments[0].begin(), arguments[0].end(), '_') + 1);
	
	//See whether we have indexes for that ngram size or not.
	auto relevant_metadata_itr = metadatas.find(ngram_size);
	//If we don't:
	if(relevant_metadata_itr == metadatas.end())
	{
		//In the case that something like 1_2_3_ was entered:
		if(arguments[0][0] && (arguments[0][arguments[0].size() - 1] ==  '_'))
		{
			cerr
				<<"invert_index: Did you mean "<<
				arguments[0].substr(0,arguments[0].size() - 1)
				<<" instead of "
				<<arguments[0]
				<<" ?"<<endl;
		}else if(arguments[0][0] == '_')
		{
		//Or maybe something like _1_2_3 was entered by mistake.
			cerr
				<<"invert_index: Did you mean "
				<<arguments[0].substr(1)
				<<" instead of "
				<<arguments[0]
				<<" ?"<<endl;
		}else
		{
			cerr<<"invert_index: Either the argument is invalid, or there doesn't exist a "<<ngram_size<<"_grams.metadata file"<<endl;
		}
		exit(-1);
	}

	Metadata &relevant_metadata = relevant_metadata_itr->second;

	//Let's check whether the index to be inverted exists or not.
	vector<unsigned int> index_as_vector;
	stringstream argument_stream(arguments[0]);
	for(size_t i = 0; i<ngram_size; i++)
	{
		unsigned int current_number;
		argument_stream>>current_number;
		index_as_vector.push_back(current_number);
		argument_stream.get();
	}
	auto index_in_question_itr = relevant_metadata.indices.find(index_as_vector);
	if(index_in_question_itr == relevant_metadata.indices.end())
	{
		cerr<<"invert_index: Error, the index by_"<<arguments[0]<<" does not exist, according the "<<ngram_size<<"_grams.metadata file"
			<<endl;
		exit(-1);
	}

	string foldername = relevant_metadata.output_folder_name + string("/") + string("by_") + arguments[0];
	string out_filename = relevant_metadata.output_folder_name + string("/")+  string("inverted_by_") + arguments[0];

	if(!relevant_metadata.max_frequency)
	{
		get_max_frequency(relevant_metadata,foldername);
	}

	long long int *table = (long long int*) calloc(relevant_metadata.max_frequency + 1,sizeof(*table));

	//We use a type of counting sort
	string filename  = foldername + string("/0.out");
	//A buffer for getline.
	char* line_buf = (char*) malloc(1024);
	size_t line_buf_size = 1024;

	size_t output_file_size = 0;

	//We use C-style files and strings etc as this possibly optimizes the loop below.
	FILE* currentfile = fopen(filename.c_str(),"r");
	if(!currentfile)
	{
		free(line_buf); free(table);
		cerr<<"Unable to open "<<filename<<" for reading"<<endl;
		exit(-1);
	}
	ssize_t read;
	while((read = getline(&line_buf,&line_buf_size,currentfile)) > 0)
	{
		//Set ptr to point to the newline after the number.
		const char* ptr = line_buf + read - 1;
		long long int number = 0;
		int current_power = 0;
		while(--ptr >= line_buf)
		{
			switch(*ptr)
			{
			case '0':;
			break;
			case '1':number += 1*integer_pow_10(current_power);
			break;
			case '2': number += 2*integer_pow_10(current_power);
			break;
			case '3':number += 3*integer_pow_10(current_power);
			break;
			case '4':number += 4*integer_pow_10(current_power);
			break;
			case '5':number += 5*integer_pow_10(current_power);
			break;
			case '6':number += 6*integer_pow_10(current_power);
			break;
			case '7':number += 7*integer_pow_10(current_power);
			break;
			case '8':number += 8*integer_pow_10(current_power);
			break;
			case '9':number += 9*integer_pow_10(current_power);
			break;
			default: goto outofloop2;
			}
			current_power++;
		}
		outofloop2:
		if((ptr == line_buf) || (number == 0))
		{
			free(line_buf); free(table);
			fprintf(stderr,"invert_index: Non-empty file to read has an invalid format\n");
			exit(-1);
		}
		table[number]++;
 		//16 characters per 64 bit hex number
		output_file_size += (16 + 1 + 16 + 1);
	}

	fclose(currentfile);
	free(line_buf);

	//We sift down:
	for(size_t i = relevant_metadata.max_frequency; i>1;i--)
		table[i -1] += table[i];

	//We open the output file, set it to the correct size and then mmap it.
	int fd = open(out_filename.c_str(),O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if(fd == -1)
	{
		free(table);
		cerr<<"invert_index: Unable to open file "<<out_filename.c_str()<<endl;
		exit(-1);
	}

	if(ftruncate(fd,output_file_size))
	{
		cerr<<"Unable to ftruncate output file"<<endl;
	}

	void* outfile_map = mmap(NULL,output_file_size,PROT_WRITE | PROT_READ,MAP_SHARED,fd,0);
	if(outfile_map == MAP_FAILED)
	{
		free(table);
		cerr<<"invert_index: Unable to mmap file "<<out_filename.c_str()<<endl;
		exit(-1);
	}

	//The last stage of the distribution sort
	line_buf_size = 1024;
	line_buf = (char*) malloc(line_buf_size);
	long long current_offset = 0;
	currentfile = fopen(filename.c_str(),"r");
	while((read = getline(&line_buf,&line_buf_size,currentfile)) > 0)
	{
		//Set ptr to point to the newline after the number.
		const char* ptr = line_buf + read - 1;
		long long int number = 0;
		int current_power = 0;
		while(--ptr >= line_buf)
		{
			switch(*ptr)
			{
			case '0':;
			break;
			case '1':number += 1*integer_pow_10(current_power);
			break;
			case '2': number += 2*integer_pow_10(current_power);
			break;
			case '3':number += 3*integer_pow_10(current_power);
			break;
			case '4':number += 4*integer_pow_10(current_power);
			break;
			case '5':number += 5*integer_pow_10(current_power);
			break;
			case '6':number += 6*integer_pow_10(current_power);
			break;
			case '7':number += 7*integer_pow_10(current_power);
			break;
			case '8':number += 8*integer_pow_10(current_power);
			break;
			case '9':number += 9*integer_pow_10(current_power);
			break;
			default: goto outofloop3;
			}
			current_power++;
		}
		outofloop3:
		//We don't check for invalid input here because we did so above, and doing so again would just be paranoid.

		long long int pos = --table[number]; 

		char* write_pos = (char*) outfile_map + pos*(16+1+16+1);
		snprintf(write_pos,(16+ 1+16+1), "%16llX\t%16llX",number, current_offset);
		write_pos[16 + 1 +16 +1 -1] = '\n';
		current_offset += read;
	}
	fclose(currentfile);
	free(line_buf);

	munmap(outfile_map,output_file_size);
	close(fd);

	//We have the index_as_vector variable from before still lying around...
	//We write the fact that we've made a new index to the n-gram metadata.
	relevant_metadata.inverted_indices.insert(index_as_vector);
	relevant_metadata.write();

	free(table);
	return;

}

