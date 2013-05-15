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

void searchable_file::search(string &search_str,vector<string> &results)
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

	stringstream foundstrings;
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
				for(itr = i; itr<mmaped_index_size ;itr++)
				{
					if((mmaped_index[itr] == '\n') && (itr < (mmaped_index_size -1)))
					{
						results.push_back(foundstrings.str());
						foundstrings.str("");
						itr++;
						if(strncmp(mmaped_index +itr, search_string,search_string_length))
						{
							break;
						}
					}
					foundstrings<<mmaped_index[itr];
				}
				if(foundstrings.str().length())
					results.push_back(foundstrings.str());

				foundstrings.str("");
				
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
							foundstrings<<mmaped_index[j];
						}
					}
					results.push_back(foundstrings.str());
					foundstrings.str("");
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
