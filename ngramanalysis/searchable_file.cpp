#include "searchable_file.h"

searchable_file::searchable_file(const string &index_filename,bool is_pos)
{
	this->is_pos = is_pos;
	this->index_filename = index_filename;
	//Let's open the file,get its size, and then mmap it.
	fd = open(index_filename.c_str(), O_RDONLY);
	if(fd < 0)
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
	if(fileinfo.st_size < 4)
	{
		cerr<<"searchable_file: Index with filename "<<index_filename<<" is a too-small file, exiting"<<endl;
		exit(-1);
	}

	this->mmaped_index = (char*) mmap(NULL,mmaped_index_size,PROT_READ, MAP_PRIVATE,fd, 0);
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

	
	//We maintain a search tree to speedup running multiple searches on the file.
	//The value of a string is somewhere within the string which is the key of the search string, or 0 for an artificially low "" key.
	//We search the file between the first key that is not lexographically less than the the search string and the key after that.
	       
	//The lowest key in the search tree.
	//Let ptr2 point to just past the end of the first key.
	const char* ptr = mmaped_index;
	const char* const mmaped_index_eof = mmaped_index + mmaped_index_size;
	int (*isdigit)(int) = std::isdigit;
	const char *ptr2 = find_if((const char*)mmaped_index,mmaped_index_eof,isdigit);
	if(ptr2 == (mmaped_index_eof))
	{
		cerr<<"searchable_file: The file "<<index_filename<<" has incorrect formatting"<<endl;
		exit(-1);
	}

	search_tree[string(ptr,ptr2)] = 0;

	//Find the last string in the file. We subtract one to make ptr point to the newline and then another one to point just before the newline.
	ptr = mmaped_index_eof - 2;

	//Point to just after second to last newline or to the start of the file, whichever comes first.
	while((ptr != mmaped_index ) && (*(ptr-1) != '\n'))
		ptr--;

	ptr2 = find_if(ptr,mmaped_index_eof,isdigit);
	if(ptr2 == mmaped_index_eof)
	{
		cerr<<"searchable_file: The file "<<index_filename<<" has incorrect formatting"<<endl;
		exit(-1);
	}
	ptr2--;

	//The string between ptr2 and ptr is the largest key in the file. 
	search_tree[string(ptr, ptr2) ] = ptr - mmaped_index;
}

//Note that search_str must include the final tab character for the search string.
void searchable_file::search(const string &search_str,vector<string> &results,const string &filter)
{
	const char* search_string = search_str.c_str();
	int search_string_length = strlen(search_string);

	if(search_string_length > mmaped_index_size)
	{
		return;
	}
	
	//Note that these quantities are signed, to allow upper to be set to -1 meaning the search finishes unsucessfully.
	int64_t upper, lower, i;

	//lower_bound "Returns an iterator pointing to the first element that is not less than key."
	//->returns search_tree.end() iff there is no element that is not less than search key,
	//->returns search_tree.end() iff all elements are not not less than search key
	//->returns search_tree.end() iff all elements are less than search key.
	//The largest element in the index is part of the search tree (see constructor)
	//=>return search_tree.end() iff there is no element in the index greater than or equal to the search key:
	auto itr = search_tree.lower_bound(search_str);
	if(itr == search_tree.end())
		return;//Nothing found.
	else
		upper = itr->second;
	//Given the upper is the first lement that is not less than key, the key before upper must be the first element
	//that is less than key. Note that in the case that upper is equal to the first key, i.e. upper == 0 (i.e. itr == search_tree.begin),
	//we set lower to 0 as our search function searches for the values inclusive.

	if(itr != search_tree.begin())
	{
		itr--;
		lower = itr->second;
	}else
		lower = 0;

	//Here we do the actual binary search.
	while(1)
	{
		if(upper < lower) //The algorithm terminates, having found nothing.
		{
			break;
		}else
		{
			//We ignore any integer overflow issues here as we're using 64-bit integers and checking for overflow would be overkill.
			i = (upper + lower)/2;

			//Go to the first character after the \n before (upper + lower)/2, or to lower, whichever comes first.
			while((i != lower) && (mmaped_index[i-1] != '\n'))
				i--;

			//Set this to right after the tab character, or the end of the line.
			int64_t end_of_string_starting_at_i;
			for(
			     end_of_string_starting_at_i = i;
			     ((end_of_string_starting_at_i+1) < mmaped_index_size) && (!isdigit(mmaped_index[end_of_string_starting_at_i+1]));
			      end_of_string_starting_at_i++
			)
				;

			//We should never be reaching the end of the file
			if((end_of_string_starting_at_i == mmaped_index_size) || (mmaped_index[end_of_string_starting_at_i] != '\t'))
			{
				cerr<<"searchable_file: Input file "<<index_filename<<" has incorrect formatting."<<endl;
				exit(-1);
			}

			//Add this string to our small index.
			search_tree[string(mmaped_index + i,mmaped_index + end_of_string_starting_at_i)] = i;

			//This works because both strings contain tabs (including at the end of the strings),
			//as delimiters.
			int cmp = strncmp(mmaped_index + i,search_string,search_string_length);
			
			if(cmp == 0)//We've found a match.
			{
				//We now get those strings which are at and after the match, later, below, we also get those from before the match.
				int64_t  itr,start_of_string_ending_at_itr; //Some loop iterators, where the latter follows the former.
				for(itr = start_of_string_ending_at_itr = i; ;itr++)
				{
					if(itr == mmaped_index_size)
					{
						cerr<<"searchable_file: File "<<index_filename<<" has incorrect formatting"<<endl;
						exit(-1);
					}
					if(mmaped_index[itr] == '\n')
					{
						string result_string(mmaped_index + start_of_string_ending_at_itr,mmaped_index + itr);
						//We can use std::move below, as we never use result_string again.
						if(string_matches_filter(result_string,filter,search_str))
							results.push_back(move(result_string));
						itr++; //We're now at just after the newline.
						if(itr + search_string_length > mmaped_index_size) //Nothing long enough here.
							break;

						//Here we check if the next string matches, if it does, we continue.
						if(strncmp(mmaped_index + itr, search_string,search_string_length))
							break;
						else
						{
							start_of_string_ending_at_itr = itr;
							itr += search_string_length;
						}
					}
				}
				
				//Here we get the strings before i.
				

				//Make sure there is something before i.
				//The minimum number of bytes for an n-gram is 4.(e.g. a\t1\n)
				if(i < 4)
					break;
				for(start_of_string_ending_at_itr = (i-2), itr = (i - 1); ;start_of_string_ending_at_itr--)
				{
					//Note that we know that in this loop, start_of_string_ending_at_itr starts at >= 2
					//and itr starts at >= 3
					if((start_of_string_ending_at_itr == 0) || (mmaped_index[start_of_string_ending_at_itr-1] == '\n'))
					{
						//We've reached the start of this string:
						if(strncmp(mmaped_index+start_of_string_ending_at_itr,search_string,search_string_length))
						{
							//The start of the string no longer matches what we're looking for.
							break;
						}else
						{
							string result_string(mmaped_index + start_of_string_ending_at_itr,mmaped_index + itr);
							if(string_matches_filter(result_string,filter,search_str))
								results.push_back(move(result_string));	
							if(start_of_string_ending_at_itr < 4)
								break;
							else
								itr = start_of_string_ending_at_itr-1;
						}
					}

				}
				//Exit the loop, we've added all of our results.
				break;

			}
			//This binary search searches all of the index file because:	
			//a) If upper == lower then that location is searched.
			//c) Ranges like [first,second] where first < second result in the search exiting unsuccessfully.
			//b) At this point in the program, the location at i has already been searched (i points to the start of a line).
			//c) Where cmp < 0, the region [next(i),upper] is searched.
			//d) Where cmp > 0, the region [lower,prev(i)] is searched unless if i == 0, in which case the search exits unsuccessfully.
			//e) i is always either lower or between upper and lower. 
			//f) It is clear that if the key is in the index, it is within all of the ranges searched.
			//g) As the ranges are always decreasing in size, and the range where both limits are equal results in
			//	either the element being found or not, we can conclude that this search finds the element if
			//	it is in the index and the index is sorted.
			else if (cmp < 0)
			{
				//Set lower forwards to the start of the next string after i.
				lower = i;
				while((lower != mmaped_index_size) && mmaped_index[lower] != '\n')
					lower++;
				if(lower == mmaped_index_size)
				{
					cerr<<"searchable_file: File "<<index_filename<<" has incorrect formatting"<<endl;
					exit(-1);
				}
				lower++;
			}else if(cmp > 0)
			{
				//If i is at the start of the file.
				if(i < 4)
				{
					//This means we have not found anything, we set upper to -1  which means we exit.
					if(i == 0)
						upper = -1;
					else
					{
						cerr<<"searchable_file: File "<<index_filename<<" seems to have too short n-grams at start"<<endl;
					}
				}else
				{
					//Set upper backwards to the start of the next string before i.
					upper = i - 2; //Point to just before the newline
					while((upper != 0) && (mmaped_index[upper-1] != '\n'))
						upper--;
				}
				//Now upper points to the start of a line, or to -1, which causes the search to end unsucessfully.
			}
			
		}
	}

}

//A destructor.
searchable_file::~searchable_file()
{
	munmap(mmaped_index, mmaped_index_size);
	close(fd);
}

/*
 * This function returns true if a certain string from the n-gram database (result_s) matches a certain filter (filter_s) and 
 * a certain search string (search_string_s).
 * The filter string must have its beginning be equal to the search string, but may be longer than the search string.
 * 	The filter string can end in something like the following, where the tab matches the single tab there is always in the results string (see below):
 * 	For non-POS corpora: 
 * 		"* someword *\t"
 * 			 where '*' matches one word (and one word only) in the end result string.
 * 	For POS corpora:
 * 		"*|something|somethingorother np|something|*\t"
 * 			where the * in this case matches a while word or lemma or type, as appropriate.
 *  	Note that the filter string must end in a tab character.
 *  	The search string must also end in a tab character.
 * 	All three strings must *not* have duplicate tab characters in them.
 * The result string must also, as its beginning, have the search string.
 * 	The result string must end in "\t<some_number>"
 */
bool searchable_file::string_matches_filter(const string &result_s, const string &filter_s,const string &search_string_s)
{
	/* Make sure that the arguments are correctly formatted,doing so helps find bugs in other areas of the code.*/

	if(filter_s.length() < search_string_s.length())
	{
		cerr<<"searchable_file: Filter string length does not match search string length"<<endl;
		exit(-1);
	}else if(result_s.length() < search_string_s.length())
	{
		cerr<<"searchable_file: Result string length does not match search string length"<<endl;
		exit(-1);
	}else if( strstr(result_s.c_str(),"\t\t"))
	{
		cerr<<"searchable_file: Result string '"<<result_s<<"' has incorrect formatting"<<endl;
		exit(-1);
	}else if( strstr(filter_s.c_str(),"\t\t") || (*filter_s.rbegin() != '\t'))
	{
		cerr<<"searchable_file: Filter string'"<<filter_s<<"'has incorrect formatting"<<endl,
		exit(-1);
	}else if( strstr(search_string_s.c_str(),"\t\t") || (*search_string_s.rbegin() != '\t'))
	{
		//Search strings *must* end in a tab.
		cerr<<"searchable_file: Search string has incorrect formatting";
		exit(-1);
	}else if(
			!strrchr(result_s.c_str(),'\t')
		       	|| 
			! all_of(strrchr(result_s.c_str(),'\t') + 1, result_s.c_str() + result_s.length(),
				[](const char &input)->bool
				{
					return (bool) isdigit(input);	
				}
			)
		)
	{
		cerr<<"searchable_file: Results string should end in a tab and then a number, this seems to not be the case"<<endl;
		exit(-1);
	}else if(result_s.substr(0,search_string_s.length()) != search_string_s)
	{
		cerr<<"searchable_file: Results string should begin with search string, this is not the case and hence there is a bug somewhere"<<endl;
		exit(-1);
	}else if(filter_s.substr(0,search_string_s.length()) != search_string_s)
	{
		cerr<<"searchable_file: Filter string should begin with search string, this is not the case and hence there is a bug somewhere"<<endl;
		exit(-1);
	}



	//Because we know that the initial portions of the search strings match, we simply start from there. We keep two pointers, one for the
	//current place in filter and one for the corresponding place in the result string.
	const char* filter_ptr;
	const char* result_ptr;
	const char* result = result_s.c_str();
	const char* filter = filter_s.c_str();
	enum {CLASSIFICATION, LEMMA, INFLEXION} current_pos_part = CLASSIFICATION;

	for(filter_ptr = filter,result_ptr = result ;;filter_ptr++)
	{
		switch(*filter_ptr)
		{
		case '\0':
			if(!isdigit(*result_ptr))
			{
				cerr<<"N-grams of incorrect length found in this index, exiting"<<endl;
				exit(-1);
			}else
			{
				return true;
			}
		break;
		case '*':
			 if(!is_pos)
			 {
				//We skip forward to the next whitespace.
				result_ptr = strchr(result_ptr, '\t');
				if(!result_ptr)
				{
					cerr<<"Expected to find tab character, did not find one"<<endl;
					exit(-1);
				}
			 }else
			 {
				 switch(current_pos_part)
				 {
					 case CLASSIFICATION: 
					 if(*result_ptr != '$')	
					 {
					 case LEMMA://Structural programmers gonna hate.
					 	result_ptr = strchr(result_ptr,'|');
						if(!result_ptr)
						{
							cerr<<"Expected to find '|' in result string, but didn't"<<endl;
							exit(-1);
						}
					 }else
					 {
						//In the case of a null word we simply increment result_ptr, as MARKER1 below doesn't check current_pos_part
						result_ptr++;
						if(*result_ptr != '\t')
						{
							cerr<<"Expected tab character in result"<<endl;
							exit(-1);
						}
						filter_ptr = strchr(filter_ptr,'\t');
						if(!filter_ptr)
						{
							cerr<<"String that should have contained tab character didn't"<<endl;
							exit(-1);
						}
						filter_ptr--;//As this gets incremented in the next iteration.
					 }
					break;
					case INFLEXION:
					result_ptr = strchr(result_ptr,'\t');
					if(!result_ptr)
					{
						cerr<<"searchable_file: Expecting tab but did not find it, exiting"<<endl;
						exit(-1);
					}
				 }
			 }
		break;

		default:
			if(*filter_ptr != *result_ptr)
			{
				return false;
			}else
			{
				if(is_pos && (*filter_ptr == '|'))
				{
					if(current_pos_part == CLASSIFICATION)
						current_pos_part = LEMMA;
					else if(current_pos_part == LEMMA)
						current_pos_part = INFLEXION;
				}else if(is_pos && (*filter_ptr == '\t'))
				{
					//MARKER1: Do not check current_pos_part before setting it.
					current_pos_part = CLASSIFICATION;
				}else if(is_pos && (*filter_ptr == '$'))
				{
					if(*(filter_ptr+1) && (*(filter_ptr+1) != '\t'))
					{
						cerr<<"Poorly constructed filter string, exiting"<<endl;
						exit(-1);
					}
					//Given that the next character in the filter string is a tab, 
					//we can rely on the fact that MARKER1 resets any POS states above.
				}
				result_ptr++;
			}
		}
	}

	return true;
}


