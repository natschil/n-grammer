#include "entropy_index.h"

static int compare_entries(const void* first, const void* second)
{
	char* endptr;
	unsigned int first_entropy_u = strtoul((const char*)first,&endptr,16);
	unsigned int second_entropy_u = strtoul((const char*)second,&endptr,16);
	return first_entropy_u - second_entropy_u;
}

void entropy_index( map<unsigned int,Metadata> &metadatas,vector<string> arguments)
{
	//TODO: Check about floating types more here.
	if(sizeof(float) != sizeof(uint32_t))
	{
		cerr<<"This machine does not have sizeof(float) == 8 , and hence this query won't work."<<endl;
		exit(-1);
	}
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
	if(*search_string.rbegin() == ' ')
	{
		cerr<<"The argument should not end in ' '"<<endl;
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

	//Only build indexes which perfectly match for n-gram size, for now:
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

	stringstream index_filename_stringstream("");
	index_filename_stringstream.seekp(0, std::ios::end);
 	for(auto i : search_index_to_use)
	{
		index_filename_stringstream<<"_";
		index_filename_stringstream<<i;
	}

	string index_filename = relevant_metadata.output_folder_name + "/by" +  index_filename_stringstream.str() + "/0.out";
	stringstream number;
	number<<unknown_words.size();
	string output_filename = 
		relevant_metadata.output_folder_name + string("/entropy_") + number.str() + string("_index") + index_filename_stringstream.str();

	//The input file:
	int in_fd = open(index_filename.c_str(), O_RDWR);
	off_t in_file_size = lseek(in_fd,0, SEEK_END);
	if(in_fd < 0 )
	{
		cerr<<"Could not open the file "<<index_filename<<endl;
		exit(-1);
	}


	const char* mmaped_in_file = (const char*) mmap(NULL, in_file_size, PROT_READ, MAP_SHARED,in_fd, 0);
	if(mmaped_in_file == (char*) -1)
	{
		cerr<<"Could not mmap the file "<<index_filename<<endl;
		exit(-1);
	}

	unlink(output_filename.c_str());
	FILE* output_file = fopen(output_filename.c_str(),"w+");
	if(!output_file)
	{
		cerr<<"Could not open the file "<<output_filename<<" for writing"<<endl;
		exit(-1);
	}
	//TODO: posix_fallocate something here,possibly.

	//Here the actual fun begins:
	
	//We determine the entropies:
		
	const char* eof = mmaped_in_file + in_file_size;

	const char* first_line = mmaped_in_file;
	size_t first_significant_part_length = 0;
	const char* end_of_first_line_significant_part = NULL;

	const char* current_line = mmaped_in_file;
	const char* end_of_current_line;

	const char* end_of_current_significant_part = NULL;
	int num_spaces = 0;
	const int num_spaces_to_end_of_significant_part = known_words.size();

	int just_getting_total_frequency = 1;

	long long int current_total_frequency = 0;
	long double current_negative_entropy = (long double)0;

	for(end_of_current_line = mmaped_in_file; end_of_current_line != eof; end_of_current_line++)
	{
		//We iterate over the mmaped_in_file.
		switch(*end_of_current_line)
		{
			case '\t': 
			{
				//We get the number after the tab.
				char* endptr;
				long long int current_frequency = strtol(end_of_current_line+1,&endptr,10);

				//We want to continue after the newline after the number. Since the loop increments this, we leave it here.
				end_of_current_line = endptr;

				//We update whatever field it is we're updating.
				if(just_getting_total_frequency)
				{
					current_total_frequency += current_frequency;	
				}else
				{
					long double p = (long double) current_frequency / (long double) current_total_frequency;
					current_negative_entropy += (p + log2l(p));
				}
				//Set the current line to after the newline.
				current_line = end_of_current_line + 1;
				num_spaces = 0;	
			}
			break;
			case ' ':
				//We want to keep track of where we are on the line.
				num_spaces++;
				//Since we're looking for everything with the starts the same, we wait until this occurs:
				if(num_spaces == num_spaces_to_end_of_significant_part)
				{
					//We update this variable, as we might need it later.
					
					end_of_current_significant_part = end_of_current_line;
					//We set this variable if it hasn't been set. TODO: Do this only once, and not check every time.
					if(!end_of_first_line_significant_part)
					{
						end_of_first_line_significant_part = end_of_current_line;
						first_significant_part_length = end_of_first_line_significant_part - first_line;
					}

					if(
						(first_significant_part_length != (size_t)(end_of_current_significant_part - current_line)) 
								||
						strncmp(first_line,current_line,first_significant_part_length)
							)
					{
						if(just_getting_total_frequency)
						{
							//We need to reset everything, except for the total frequency.
							
							current_line = first_line;
							end_of_current_line = end_of_first_line_significant_part;
							end_of_current_significant_part = end_of_first_line_significant_part;

						}else
						{
							float tmp_entropy = (-1 * current_negative_entropy);
							if(tmp_entropy == -0.0)
								tmp_entropy = 0;
							//We output our floats as unsigned, 32 bit integers, as that makes
							//width specifiers easier. Additionally, given that all of our floats
							//(should) be non-negative, sorting them doesn't require all of the float
							//overhead.
							fprintf(output_file,"%8X %16lX\n",
									*reinterpret_cast<uint32_t*>(&tmp_entropy),
									(uint64_t) (first_line - mmaped_in_file)
							       );

							first_line = current_line;
							end_of_first_line_significant_part = end_of_current_significant_part;
							first_significant_part_length = end_of_first_line_significant_part - first_line;

							current_total_frequency = 0;
						 	current_negative_entropy = 0;
						}
						just_getting_total_frequency = !just_getting_total_frequency;
					}
					
				}
				
		}
	}
	//Let's open the output file.
	munmap((void*)mmaped_in_file,in_file_size);
	close(in_fd);
	fflush(output_file);
	off_t output_file_size = ftell(output_file);
	int output_file_fd = fileno(output_file);
	char* mmaped_output_file = (char*)mmap(NULL,output_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, output_file_fd, 0);
	if(mmaped_output_file == (char*) -1)
	{
		cerr<<"Could not mmap output file"<<output_filename<<endl;
		exit(-1);
	}
	char* outfile_eof = mmaped_output_file + output_file_size;
	qsort(mmaped_output_file,output_file_size/(8 + 1 + 16 + 1),(8 + 1 + 16 + 1),&compare_entries);

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
		for(output_file_ptr = current_stackptr->begin; output_file_ptr != current_stackptr->end;output_file_ptr+=(8 + 1 + 16 + 1))
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
				stackptrs[depth+1]->begin = mmaped_output_file + (*cur * (8 + 1 + 16 + 1));
				stackptrs[depth+1]->end = mmaped_output_file + (total * (8 + 1 + 16 + 1));
				stackptrs[depth+1]++;
			}
		}
		memcpy(table_backup,table,((1l<<16l)+1)*sizeof(*table_backup));
		//Then we fill the buckets:
		for(unsigned int i = 0; i < (1l<<16l); i++)
		{
			if(!(table_backup[i+1] - table[i]))
				continue;
			char* original_location  = (8 + 1 + 16 + 1)*(table_backup[i+1] - 1) + mmaped_output_file;

			while(1)
			{
				char* endptr;
				uint32_t tmp_u = strtoul(original_location,&endptr,16);
				uint32_t relevant_bits = (tmp_u >> shift) & 0xFFFF;
				char* next = (mmaped_output_file) + (8 + 1 + 16 + 1)*((table[relevant_bits])++);
				if(next == original_location)
					break;
				else
				{
					//We swap the two elements:
					char tmp[8 + 1 + 16 + 1];
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
	cerr<<"Finished building entropy index"<<endl;	
	munmap(mmaped_output_file,output_file_size);
	fclose(output_file);

}
