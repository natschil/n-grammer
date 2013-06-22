#include "searchable_file.h"

searchable_file::searchable_file(string &index_filename,bool is_pos)
{
	this->is_pos = is_pos;
	this->index_filename = index_filename;
	//Let's open the file,get its size, and then mmap it.
	fd = open(index_filename.c_str(), O_RDONLY);
	if(fd == -1)
	{
		cerr<<"searchable_file: Unable to open index " << index_filename<<endl;
		exit(-1);
	}
	struct stat fileinfo;
	if(stat(index_filename.c_str(),&fileinfo))
	{
		cerr<<"searchable_file: Unable to stat index " << index_filename<<endl;
		exit(-1);
	}
	this->mmaped_index_size = fileinfo.st_size;

	mmaped_index = (char*) mmap(NULL,mmaped_index_size,PROT_READ, MAP_PRIVATE,fd, 0);
	if(mmaped_index == (void*) -1)
	{
		cerr<<"searchable_file: Unable to mmap index " <<index_filename<<endl;
		exit(-1);
	}
	if(posix_madvise(mmaped_index,mmaped_index_size, MADV_RANDOM))
	{
		cerr<<"searchable_file: madvise failed, this should probably be looked into."<<endl;
		exit(-1);
	}

	const char* ptr = mmaped_index;
	
	//We maintain a search tree to speedup running multiple searches on the file.
	//The value of a string is somewhere within the string which is the key of the search string, or 0 for an artificially low "" key.
	//We search the file between the first key that is not lexographically less than the the search string and the key after that.
	       
	//The lowest key in the search tree.
	//Let ptr2 point to just past the end of the first key.
	const char *ptr2 = ptr+1;
	while(*ptr2 != '\t')
	{
		if(ptr2 == (mmaped_index + mmaped_index_size))
		{
			cerr<<"searchable_file: The file "<<index_filename<<" has incorrect formatting";
			exit(-1);
		}
		ptr2++;
	}

	search_tree[string(ptr,ptr2)] = 0;

	//Find the last string in the file. We subtract one to make ptr point to the newline and then another one to point just before the newline.
	ptr = mmaped_index + mmaped_index_size - 2;
	//Point to just after second to last newline or to the start of the file, whichever comes first.
	while((ptr >= 0) && (*ptr != '\n'))
		ptr--;
	ptr++;

	//Let ptr2 point to the tab.
	ptr2 = ptr + 1;
	while(*ptr2 != '\t')
	{
		if(ptr2 == (mmaped_index + mmaped_index_size))
		{
			cerr<<"searchable_file: The file "<<index_filename<<" has incorrect formatting";
			exit(-1);
		}
		ptr2++;
	}
	//The string between ptr2 and ptr is the largest key in the file. 
	search_tree[string(ptr, ptr2) ] = ptr - mmaped_index;
}

void searchable_file::search(string &search_str,vector<string> &results,string &filter)
{

	const char* search_string = search_str.c_str();
	int search_string_length = strlen(search_string);

	if(search_string_length > mmaped_index_size)
	{
		return;
	}
	
	int64_t upper, lower, i;

	auto itr = search_tree.lower_bound(search_str);
	if(itr == search_tree.end())
		return;

	upper = (*itr).second;

	if(itr == search_tree.begin())
		lower = 0;
	else
	{
		itr--;
		lower = (*itr).second;
	}

	while(1)
	{
		if(upper<lower)
		{
			break;
		}else
		{
			i = (upper + lower)/2;
			while((i >= lower) && (mmaped_index[i] != '\n'))
				i--;
			//Go to the first character after the \n before (upper + lower)/2, or to lower, whichever comes first.
			i++;


			int64_t end_of_string_starting_at_i = i;
			for(end_of_string_starting_at_i = i;end_of_string_starting_at_i < mmaped_index_size; end_of_string_starting_at_i++)
			{
				if(mmaped_index[end_of_string_starting_at_i] == '\t')
				{
					break;
				}
			}
			if(end_of_string_starting_at_i == mmaped_index_size)
			{
				cerr<<"Input file "<<index_filename<<" has incorrect formatting."<<endl;
				exit(-1);
			}

			search_tree[string(mmaped_index +i,mmaped_index + end_of_string_starting_at_i)] = i;

			int cmp = strncmp(mmaped_index + i,search_string,search_string_length);
			if(cmp == 0)
			{
				int64_t  itr;
				string result_string;
				stringstream foundstring;

				//At first get the strings matching the filter after i
				for(itr = i; ;itr++)
				{
					if(itr == mmaped_index_size)
					{
						cerr<<"File "<<index_filename<<" has incorrect formatting"<<endl;
						exit(-1);
					}
					if(mmaped_index[itr] != '\n')
						foundstring<<mmaped_index[itr];
					else
					{
						result_string = foundstring.str();

						if(string_matches_filter(result_string,filter,search_str))
							results.push_back(result_string);

						foundstring.str("");

						itr++;//Point to after the newline.
						if((itr + search_string_length > mmaped_index_size) 
								|| strncmp(mmaped_index + itr, search_string,search_string_length))
						{
							break;
						}
						itr--;//Point to before the newline again, as the loop increments this.
					}
				}
				
				//Now get the matching strings before i.
				while(i)
				{
					//Start just before the newline, go backwards until the start of an n-gram.
					for(itr = i - 2; itr > 0; itr--)
					{
						if(mmaped_index[itr] == '\n')
						{
							itr++;
							break;
						}
					}
					//If it doesn't match, we stop looping
					if((itr + search_string_length > mmaped_index_size) 
							|| strncmp(mmaped_index + itr, search_string,search_string_length))
						break;
					else
					{
						//We go until the newline.
						int64_t j;
						for(j = itr; mmaped_index[j] != '\n';j++)
						{
							if(j == mmaped_index_size)
							{
								cerr<<"File "<<index_filename<<" has incorrect format"<<endl;
							}
							foundstring<<mmaped_index[j];
						}
					}
					result_string = foundstring.str();
					if(string_matches_filter(result_string,filter,search_str))
						results.push_back(result_string);

					foundstring.str("");
					i = itr;
				}
				//Exit the loop, we've added all of our results.
				break;

			}else if (cmp < 0)
			{
				//Set lower forwards to the start of the next string after i.
				lower = i;
				while(mmaped_index[lower] != '\n')
					lower++;
				lower++;
			}else if(cmp > 0)
			{
				//Set upper backwards to the start of the next string before i.
				upper = i - 2;
				if(upper > 0)
				{
					while(mmaped_index[upper] >= 0)
					{
						if(mmaped_index[upper] == '\n')
							break;
						upper--;
					}
					upper++;
				}
			}
			
		}
	}

}

searchable_file::~searchable_file()
{
	munmap(mmaped_index, mmaped_index_size);
	close(fd);
}

bool searchable_file::string_matches_filter(string &result_s, string &filter_s,string &search_string_s)
{
	const char* result = result_s.c_str();
	const char* filter = filter_s.c_str();
	const char* search_string = search_string_s.c_str();

	if(strlen(filter) < strlen(search_string))
	{
		cerr<<"There is a bug somewhere..."<<endl;
		exit(-1);
	}
	if(strlen(filter) == strlen(search_string))
	{
		return 1;
	}
	if(strlen(result) < strlen(search_string))
	{
		cerr<<"This indicates that there is a bug"<<endl;
		exit(-1);
	}

	const char* filter_ptr;
	const char* result_ptr;
	for(filter_ptr= filter + strlen(search_string),result_ptr = result + strlen(search_string);; filter_ptr++,result_ptr++)
	{
		if(!*filter_ptr)
		{
			if(*result_ptr)
				result_ptr++;
			else
			{
				cerr<<"It looks like there is a bug here"<<endl;
				return 0;
			}

			if(isdigit(*result_ptr))
			{
				break;
			}else
			{
				cerr<<"This is a (probably serious) bug in the filter mechanism of search for filter."<<endl;
				return 0;
			}
		}
		if(!*result_ptr)
		{
			cerr<<"This is a bug"<<endl;
			exit(-1);
		}
		if(*filter_ptr == ' ')
		{
			continue;
		}

		if(*filter_ptr == '*')
		{
			//TODO: optimize this.
			if(!is_pos)
			{
				const char* result_tmp = strpbrk(result_ptr," \t");
				if(result_tmp)
					result_ptr = result_tmp;
				else
					result_ptr = result_ptr + strlen(result_ptr);
			}else
			{
				const char* result_tmp = strpbrk(result_ptr,"| \t");
				if(result_tmp)
					result_ptr = result_tmp;
				else
					result_ptr = result_ptr + strlen(result_ptr);
			}
			result_ptr--;
			continue;
		}

		if(*filter_ptr != *result_ptr)
			return 0;		
	}

	return 1;
}


