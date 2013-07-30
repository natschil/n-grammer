#include "entropy_index.h"

const char entropy_index_header[] = "Entropy Index File in Binary Format\n---Begin Binary Reference Info---\n";
const char after_entropy_index_test[] = "\n---End Binary Reference Info---\n---Actual Index is in Binary Form Below---\n";

static int compare_entries(const void* first, const void* second)
{

	const float  *first_entropy = (const float*)first;
	const float *second_entropy = (const float*) second;
	return (*first_entropy > *second_entropy) - (*first_entropy < *second_entropy);
}

void entropy_index( map<unsigned int,Metadata> &metadatas,vector<string> arguments)
{
    //We deal with the arguments, making sure they have the correct format:
	if(arguments.size() != 1)
	{
		cerr<<"Please give one and only one argument to entropy_index"<<endl;
		exit(-1);
	}
	string search_string = arguments[0];
	if(strspn(search_string.c_str()," *=") != search_string.length())
	{
		cerr<<"Please give an argument like \"* = * = ="<<endl;
	}
	if(strspn(search_string.c_str()," *=") != search_string.length())
	{
		cerr<<"Please give an argument like \"* = * = ="<<endl;
	}

	vector<string> tokenized_string = split_ignoring_empty(search_string,' ');
	vector<unsigned int> known_words;
	vector<unsigned int> unknown_words;
	for(size_t i = 0; i< tokenized_string.size(); i++)
	{
		string &cur = tokenized_string[i];
		if(cur.length() != 1)
		{
			cerr<<"Token "<<cur<<" is too long"<<endl;
			exit(-1);
		}
		switch(cur[0])
		{
			case '*': unknown_words.push_back(i);
			break;
			case '=': known_words.push_back(i);
		}
	}

    //We now get the relevant ngram frequency index to build this kind of entropy index.
	unsigned int ngramsize = known_words.size() + unknown_words.size();

	//Only build indexes which perfectly match for n-gram size, for now. TODO: Think about whether we can do so differently.
	auto relevant_metadata_itr = metadatas.find(ngramsize);
	if(relevant_metadata_itr == metadatas.end())
	{
		cerr<<"Could not find metadata for "<<ngramsize<<"-grams"<<endl;
		exit(-1);
	}
	Metadata &relevant_metadata = relevant_metadata_itr->second;

	unsigned int matches = 0;
	vector<unsigned int> search_index_to_use;
	for(auto i = relevant_metadata.indices.begin(); i != relevant_metadata.indices.end(); i++)
	{
		unsigned int current_matches = 0;	
		for(auto j = i->begin(); j != i->end(); j++)
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
		cerr<<"Could not find a search index out of which to build this kind of entropy query.\n"<<endl;
		exit(-1);
	}


	string index_specifier = join(functional_map(search_index_to_use,unsigned_int_to_string),"_");
	string frequency_index_filename = relevant_metadata.output_folder_name + "/by_" +  index_specifier + "/0.out";
	string entropy_index_filename = relevant_metadata.output_folder_name +
	      			 	"/entropy_" + to_string(known_words.size()) + "_index_" + index_specifier;

    //We now open all of the files and mmaped regions which we will need for the sorting.
    
	//The frequency index file:
	int frequency_index_fd = open(frequency_index_filename.c_str(), O_RDWR);
	off_t frequency_index_file_size = lseek(frequency_index_fd,0, SEEK_END);
	if(frequency_index_fd < 0 )
	{
		cerr<<"Could not open the file "<<frequency_index_filename<<endl;
		exit(-1);
	}

	const char* mmaped_frequency_index = (const char*) mmap(NULL, frequency_index_file_size, PROT_READ, MAP_SHARED,frequency_index_fd, 0);
	if(mmaped_frequency_index == (const char*) -1)
	{
		cerr<<"Could not mmap the file "<<frequency_index_filename<<endl;
		exit(-1);
	}
	//The entropy index file, which we make sure to remove if it already exists:
	unlink(entropy_index_filename.c_str());
	FILE* entropy_index_file = fopen(entropy_index_filename.c_str(),"w+");
	if(!entropy_index_file)
	{
		cerr<<"Could not open the file "<<entropy_index_filename<<" for writing"<<endl;
		exit(-1);
	}

    //We now write the header part of the index, useful for knowing what the file does and whether the same
    //(nonportable) binary format was used for index creation as reading:
	fwrite(entropy_index_header,sizeof(char),sizeof(entropy_index_header) - 1,entropy_index_file);
	float test_value_float = 123.125;
	uint64_t test_value_uint = 1234567890;
	fwrite(&test_value_float,sizeof(test_value_float),1,entropy_index_file);
	fwrite(&test_value_uint, sizeof(test_value_uint),1,entropy_index_file);
	fwrite(after_entropy_index_test,sizeof(char),sizeof(after_entropy_index_test)-1,entropy_index_file);

   //Some useful constants and variables
   
	const char* const mmaped_frequency_index_eof = mmaped_frequency_index + frequency_index_file_size;

	//Refers to the first line which matches the same search string as the current line given the
	//search string format given as a parameter to this file.
	//The "significant_part" is that part of the string that matches the non-wildcard parts of the search string.
	const char* first_line_start = mmaped_frequency_index;
	const char* first_line_end_of_significant_part = find_nth(mmaped_frequency_index,mmaped_frequency_index_eof,'\t',known_words.size());
	auto first_line_significant_part_length = first_line_end_of_significant_part - first_line_start;

	//A kind of loop variable:
	const char* current_line_start = mmaped_frequency_index;
	const char* current_line_end_of_significant_part = NULL;
	const char* current_line_iterator = NULL; //Gets initialized in for loop below.

   //Here the actual fun begins:
   
	unsigned int num_words = 0; //How many spaces this line has had so far.
	const unsigned int num_words_to_end_of_significant_part = known_words.size(); //After how many spaces the "significant_part" ends.
	bool just_getting_total_frequency = true; //Is the current iteration only getting the frequencies to find out the total frequency?

	long long int current_total_frequency = 0;//The total frequency of this particular search string.
	long double current_negative_entropy = 0; //Multiple by -1 to get the entropy (when done).

	for(current_line_iterator = mmaped_frequency_index;;  current_line_iterator++)
	{
		if(current_line_iterator == mmaped_frequency_index_eof)
		{
			num_words = num_words_to_end_of_significant_part;
			current_line_start = current_line_iterator;
			current_line_end_of_significant_part = current_line_iterator;
			goto write_out_last_value;
		}else if(*current_line_iterator == '\t')
		{
			num_words++;
			if(num_words == ngramsize)//We're at just before the number.
			{
				//We get the number after the tab.
				char* endptr;
				long long int current_frequency = strtol(current_line_iterator+1,&endptr,10);
				if((*endptr != '\n') || (endptr == (current_line_iterator+1)))
				{
					fprintf(stderr,"Invalid number read from file, exiting\n");
					exit(-1);
				}

				//We want to continue after the newline after the number. Since the loop increments this, we leave it here.
				current_line_iterator = endptr;

				//We update whatever field it is we're updating.
				if(just_getting_total_frequency)
				{
					current_total_frequency += current_frequency;	
				}else
				{
					long double p = (long double) current_frequency / (long double) current_total_frequency;
					current_negative_entropy += (p * log2l(p));
				}
			//Set the current line to after the newline.
				current_line_start = current_line_iterator + 1;
				num_words = 0;	
				
			}else if(num_words == num_words_to_end_of_significant_part)
			{
				//Since we're looking for everything with the starts the same, we wait until this occurs:
				//We update this variable, as we might need it later.
				current_line_end_of_significant_part = current_line_iterator;

				//If this current significant part is different from the first one:
				if(
					(first_line_significant_part_length != (current_line_end_of_significant_part - current_line_start)) 
							||
					strncmp(first_line_start,current_line_start,first_line_significant_part_length)
						)
				{
write_out_last_value:
					if(just_getting_total_frequency)
					{
						//We need to reset everything, except for the total frequency.
						current_line_start = first_line_start;
						//We can skip the first part of the string here, as
						//	a) There is always something after the significant part 
						//	b) We only look at the frequencies at the end of the line.
						current_line_end_of_significant_part = first_line_end_of_significant_part;
						current_line_iterator = first_line_end_of_significant_part;
						
							 
					}else
					{
						float tmp_entropy = (-1 * current_negative_entropy);
						if(tmp_entropy == -0.0)
							tmp_entropy = 0;
						int classification = fpclassify(tmp_entropy);
						if(signbit(tmp_entropy))
						{
							fprintf(stderr,"Found negative entropy value %f, looks like a bug\n",tmp_entropy);
						}else if(!((classification == FP_ZERO) || (classification == FP_NORMAL) || (classification == FP_SUBNORMAL)))
						{
							fprintf(stderr,"Found non-finite entropy value %f , could be a bug\n",tmp_entropy);
							string offending_value(current_line_start,current_line_end_of_significant_part);
							fprintf(stderr,"Offending value is %s\n",offending_value.c_str());
						}else
						{
							if(fwrite((void*) &tmp_entropy,sizeof(tmp_entropy),1,entropy_index_file) != 1)
							{
								fprintf(stderr,"Failed to write float to entropy index file\n");
								exit(-1);
							}
							uint64_t offset = first_line_start - mmaped_frequency_index;
							if(fwrite((void*) &offset,sizeof(offset),1,entropy_index_file) != 1)
							{
								fprintf(stderr,"Failed to write uint64_t to entropy_index_file\n");	
								exit(-1);
							}
						}
						first_line_start = current_line_start;
						first_line_end_of_significant_part = current_line_end_of_significant_part;
						first_line_significant_part_length = first_line_end_of_significant_part - first_line_start;

						current_total_frequency = 0;
						current_negative_entropy = 0;
					}
					just_getting_total_frequency = !just_getting_total_frequency;
					if(current_line_iterator == mmaped_frequency_index_eof)
						break;
				}

				}
		}
	}
	//Let's open the output file.
	munmap((void*)mmaped_frequency_index,frequency_index_file_size);
	close(frequency_index_fd);
	fflush(entropy_index_file);

	off_t entropy_index_file_size = ftell(entropy_index_file);
	int entropy_index_fd = fileno(entropy_index_file);
	char* mmaped_entropy_index_file = (char*)mmap(NULL,entropy_index_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, entropy_index_fd, 0);
	if(mmaped_entropy_index_file == (char*) -1)
	{
		cerr<<"Could not mmap output file"<<entropy_index_filename<<endl;
		exit(-1);
	}
	constexpr size_t unit_size = (sizeof(float) + sizeof(uint64_t));
	constexpr size_t how_many_to_skip = sizeof(entropy_index_header) -1 + unit_size + sizeof(after_entropy_index_test) -1;
	char* to_sort_start = mmaped_entropy_index_file + how_many_to_skip;
	qsort(to_sort_start,(entropy_index_file_size - how_many_to_skip)/unit_size,unit_size,&compare_entries);

	/*
	//We now american flag sort the whole thing. We know that all entropies should be positive, and we're assuming ieee 754 floats,which we 
	//wrote in hex to the output file.
	//We use 16 bits per pass,for two passes, because we can.
	uint64_t* const table = (uint64_t*) calloc((1l<<16l) + 1 ,sizeof(*table));
	uint64_t* const table_backup = (uint64_t*) malloc(((1l<<16l) + 1)* sizeof(*table_backup));
	uint64_t* const end_of_table = table + ((1l<<16l) + 1);

	if(!table || !table_backup)
	{
		cerr<<"Could not allocate memory for auxillary sorting table"<<endl;
		munmap(mmaped_output_file,output_file_size);
		exit(-1);
	}

	const int lowest_depth = 1;
	struct stack_item {char *begin; char *end; } 
		*stacks =(struct stack_item*) malloc((1l << 16l)*(lowest_depth + 1)*sizeof(*stacks));

	if(!stacks)
	{
		cerr<<"Could not allocate memory for stack needed for sorting"<<endl;
		munmap(mmaped_output_file,output_file_size);
		exit(-1);
	}
	struct stack_item *(stackptrs[3]);
	struct stack_item first_stack_item;
	first_stack_item.begin = mmaped_output_file;
	first_stack_item.end = outfile_eof;
	stackptrs[0] = &first_stack_item + 1;

	for(size_t i = 1; i<(lowest_depth+2); i++)
	{
		stackptrs[i] = stacks + (1<<16)*(i-1) + 1;
	}

	int depth = 0;
	do
	{
		unsigned int shift = (1 - depth)*16; 
		struct stack_item* current_stackptr = --(stackptrs[depth]);
		//If the current stack is empty, decrease the depth and try again:
		if(current_stackptr == (stacks + (1<<16)*depth))
		{
			depth--;
			continue;
		}
		char *output_file_ptr;

		//At first we count:
		for(output_file_ptr = current_stackptr->begin; output_file_ptr != current_stackptr->end;output_file_ptr+=(16 + 1 + 16 + 1))
		{
			char* endptr;
			uint32_t entropy_u = strtoul(output_file_ptr,&endptr,16);
			unsigned int table_offset = (entropy_u >> shift) & 0xFFFF;
			table[table_offset]++;
		}
		//Then we sift up:
		uint64_t total = 0;
		for(uint64_t* cur = table; cur != end_of_table; cur++)
		{
			uint64_t prev = *cur;
			*cur = total;
			total += prev;
			if((depth != lowest_depth) && prev)
			{
				stackptrs[depth+1]->begin = mmaped_output_file + (*cur * (16 + 1 + 16 + 1));
				stackptrs[depth+1]->end = mmaped_output_file + (total * (16 + 1 + 16 + 1));
				stackptrs[depth+1]++;
			}
		}
		memcpy(table_backup,table,((1l<<16l)+1)*sizeof(*table_backup));
		//Then we fill the buckets:
		for(unsigned int i = 0; i < (1l<<16l); i++)
		{
			if(!(table_backup[i+1] - table[i]))
				continue;
			char* original_location  = (16 + 1 + 16 + 1)*(table_backup[i+1] - 1) + mmaped_output_file;

			while(1)
			{
				char* endptr;
				uint32_t tmp_u = strtoul(original_location,&endptr,16);
				uint32_t relevant_bits = (tmp_u >> shift) & 0xFFFF;
				char* next = (mmaped_output_file) + (16 + 1 + 16 + 1)*((table[relevant_bits])++);
				if(next == original_location)
					break;
				else
				{
					//We swap the two elements:
					char tmp[16 + 1 + 16 + 1];
					memcpy(tmp,next,sizeof(tmp));
					memcpy(next,original_location,sizeof(tmp));
					memcpy(original_location,tmp,sizeof(tmp));
				}

			}
			cout<<"yeah";
			cout.flush();
		}
		depth++;//Go deeper to the next level.
		memset(table,((1l<<16l)+1)*sizeof(*table),0);
	}while(depth > 0);

	//Aaand the file is sorted.
	free(table);
	*/

	pair<unsigned int,vector<unsigned int> > result(known_words.size(),search_index_to_use);
	relevant_metadata.entropy_indexes.insert(result);
	relevant_metadata.write();

	cerr<<"Finished building entropy index"<<endl;	
	munmap(mmaped_entropy_index_file,entropy_index_file_size);
	fclose(entropy_index_file);

}
