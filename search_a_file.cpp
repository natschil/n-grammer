#include "search_a_file.h"

searchable_file::searchable_file(string &index_filename)
{
	//Let's open the file,get its size, and then mmap it.
	fd = open(index_filename.c_str(), O_RDWR);
	if(fd == -1)
	{
		cerr<<"search: Unable to open index" << index_filename<<endl;
		exit(-1);
	}
	struct stat fileinfo;
	if(stat(index_filename.c_str(),&fileinfo))
	{
		cerr<<"search: Unable to stat index" << index_filename<<endl;
		exit(-1);
	}
	mmaped_index_size = fileinfo.st_size;
	

	mmaped_index = (char*) mmap(NULL,mmaped_index_size,PROT_READ, MAP_PRIVATE,fd, 0);
	if(mmaped_index == (void*) -1)
	{
		cerr<<"Unable to mmap index " <<index_filename<<endl;
		exit(-1);
	}
	if(madvise(mmaped_index,mmaped_index_size, MADV_RANDOM))
	{
		cerr<<"Madvise failed, this should probably be looked into"<<endl;
	}


	const char* ptr = mmaped_index;
	const char* ptr2 = ptr;
	while(ptr2 < (mmaped_index + mmaped_index_size))
	{
		if(*ptr2== '\t')
			break;
		ptr2++;
	}
	       
	search_tree[string(ptr, ptr2 - ptr)] = 0;

	ptr = mmaped_index + mmaped_index_size - 2;
	//Find the last string in the file.
	while((ptr > mmaped_index) && (*ptr != '\n'))
		ptr--;
	ptr++;

	ptr2 = ptr;
	while(ptr2 < (mmaped_index + mmaped_index_size))
	{
		if(*ptr2== '\t')
			break;
		ptr2++;
	}

	search_tree[string(ptr, ptr2 - ptr)] = mmaped_index_size - 1;
}

searchable_file::~searchable_file()
{
	munmap(mmaped_index, mmaped_index_size);
	close(fd);
}

int string_matches_filter(string &result_s, string &filter_s,string &search_string_s)
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
				cerr<<"This is a bug in the filter mechanism of search"<<endl;
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
			if(*strchrnul(result_ptr, ' '))
				result_ptr = strchrnul(result_ptr, ' ');
			else
				result_ptr = strchrnul(result_ptr,'\t');

			result_ptr--;
			continue;
		}

		if(*filter_ptr != *result_ptr)
			return 0;		
	}

	return 1;
}

void searchable_file::search(string &search_str,vector<string> &results,string &filter)
{

	const char* search_string = search_str.c_str();
	int search_string_length = strlen(search_string);

	if(search_string_length > mmaped_index_size)
	{
		return;
	}
	
	
	map<string,off_t>::iterator itr = search_tree.lower_bound(string(search_string));
	if(itr == search_tree.end())
	{
		return;
	}
	int64_t upper = (*itr).second;
	itr--;
	if(itr == search_tree.end())
	{
		return;
	}
	int64_t lower = (*itr).second;
	int64_t i;

	stringstream foundstring;
	while(1)
	{
		if(upper<lower)
		{
			cerr<<"No matches found"<<endl;
			break;
		}else
		{
			i = (upper + lower)/2;
			while((i >= lower) && (mmaped_index[i] != '\n'))
				i--;
			//Go to the first thing after the \n, or to the lowest thing.
			i++;
			uint64_t string_length = 0;

			search_tree[string(mmaped_index +i,search_string_length)] = i;
			int cmp = strncmp(mmaped_index + i,search_string,search_string_length);
			if(cmp == 0)
			{
				int64_t  itr;
				string result_string;
				for(itr = i; itr<mmaped_index_size ;itr++)
				{
					if((mmaped_index[itr] == '\n') && (itr < (mmaped_index_size -1)))
					{
						result_string = foundstring.str();

						if(string_matches_filter(result_string,filter,search_str))
							results.push_back(result_string);
						foundstring.str("");
						itr++;
						if(strncmp(mmaped_index +itr, search_string,search_string_length))
						{
							break;
						}
					}
					foundstring<<mmaped_index[itr];
				}
				result_string = foundstring.str();
				if(result_string.length() && string_matches_filter(result_string,filter,search_str))
					results.push_back(result_string);

				foundstring.str("");
				
				while(i)
				{
					for(itr = i - 2; itr > 0; itr--)
					{
						if(mmaped_index[itr] == '\n')
						{
							itr++;
							break;
						}
					}
					if(strncmp(mmaped_index + itr, search_string,search_string_length))
						break;
					else
					{
						int64_t j;
						for(j = itr; mmaped_index[j] != '\n';j++)
						{
							foundstring<<mmaped_index[j];
						}
					}
					result_string = foundstring.str();
					if(string_matches_filter(result_string,filter,search_str))
						results.push_back(result_string);

					foundstring.str("");
					i = itr;
				}
				break;

			}else if (cmp < 0)
			{
				lower = i + string_length;
				while(mmaped_index[lower] != '\n')
					lower++;
				lower++;
			}else if(cmp > 0)
			{
				upper = i - 1;
			}
			
		}
	}

}
