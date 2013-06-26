#include "search.h"

using namespace std;

//This is taken directly from the ngram.counting.cpp file of the n-gram conter.
static int get_next_ucs4t_from_file( const uint8_t (**ptr),const uint8_t* eof,ucs4_t *character)
{
	int read = 0;
	while(eof > *ptr)
	{
		read = u8_mbtouc(character,*ptr,eof-*ptr);
		if(read > 0)
			break;
		else
			(*ptr)++;
	}

	if(eof <= *ptr)
	{
		return 0;
	}else
	{
		(*ptr) += read;
	}

	return 1;
}

/*
 * This function both checks that the format of the POS search string is correct (i.e. doesn't contain things like [this|is|a|test])
 * and also replaces occurances of * not within square brackets with [*|*|*] and words not within square brackets with [*|*|word]
 * It normalizes the strings using the unicode NFKD normalization form and does other normalization, such as removing preceding hyphens
 * It normalizes the strings using the unicode NFKD normalization form and does other normalization, such as removing preceding hyphens
 * in the same way that getnextwordandaddit() does in ngram.counting of the n-gram counter.
 */

static void normalize_and_check_search_string(string& search_string_str,const Metadata &relevant_metadata)
{
	enum {IN_WHITESPACE,IN_CLASSIFICATION, IN_LEMMA,IN_INFLEXION,IN_SINGLE_WORD} state = IN_WHITESPACE;
	string final_search_str;
	ucs4_t character;

	size_t word_size = max(max(relevant_metadata.max_word_size,relevant_metadata.max_classification_size),relevant_metadata.max_lemma_size);
	size_t current_word_size = 0;
	uint8_t* word = (uint8_t*)malloc(word_size);
	
	const uint8_t* ptr = (const uint8_t*) search_string_str.c_str();
	const uint8_t* eof = ptr + strlen((const char*) ptr);

	while(1)
	{
		int finished = !get_next_ucs4t_from_file(&ptr,eof,&character);
		if(finished || uc_is_property_white_space(character))
		{
			if(state != IN_WHITESPACE)
			{
				if(state == IN_SINGLE_WORD)
				{
					if(((word[0] == '$') || (word[0] == '*')) && (current_word_size != 1))
					{
						cerr<<"'$' or '*' may not be part of a word"<<endl;
						free(word);
						exit(-1);
					}


					if(relevant_metadata.is_pos && (word[0] != '$' ))
						final_search_str += "[*|*|";
					uint8_t* normalized_word = (uint8_t*) malloc(word_size);
					size_t normalized_word_size = word_size;	
	
					uint8_t* normalization_result = u8_normalize(UNINORM_NFKD,word,current_word_size,normalized_word, &normalized_word_size);
					if(normalization_result > 0)
					{
						final_search_str += string((const char*) normalized_word, normalized_word_size);
						current_word_size = 0;
					}else
					{
						cerr<<"A word in the search string is longer than the maximum size in this index, and hence no results would be found"<<endl;
						free(normalized_word);
						free(word);
						exit(-1);
					}
					free(normalized_word);

					if(relevant_metadata.is_pos && (word[0] != '$'))
						final_search_str += ']';

					state = IN_WHITESPACE;
					if(!finished)
						final_search_str += ' ';
				}else
				{
					cerr<<"Unexpected whitespace in search string, exiting"<<endl;	
					free(word);
					
					exit(-1);
				}
				if(finished)
					break;
			}
		}else if(character == '|')
		{
			switch(state)
			{
			case IN_WHITESPACE: 	cerr<<"Search string has invalid format, expected '[' instead of '|'"<<endl;
					    	free(word);
					    	exit(-1);
			break;
			case IN_CLASSIFICATION: state = IN_LEMMA; 
			break;
			case IN_LEMMA: 		state = IN_INFLEXION; 
			break;
			case IN_INFLEXION: 	cerr<<"Search string has invalid format, expected ']' instead of '|'"<<endl;
					   	free(word);
					   	exit(-1);
			break;
			case IN_SINGLE_WORD:	cerr<<"Search string has invalid format, expected ' ' instead of '|'"<<endl;
						free(word);
						exit(-1);
			}
			uint8_t* normalized_word = (uint8_t*) malloc(word_size);
			size_t normalized_word_size = word_size;	

			uint8_t* normalization_result = u8_normalize(UNINORM_NFKD,word,current_word_size,normalized_word, &normalized_word_size);
			if(normalization_result > 0)
			{
				final_search_str += string((const char*) normalized_word, normalized_word_size);
				current_word_size = 0;
			}else
			{
				cerr<<"A word in the search string is longer than the maximum size in this index, and hence no results would be found"<<endl;
				free(normalized_word);
				free(word);
				exit(-1);
			}
			free(normalized_word);

			final_search_str += '|';

		}else if(character == '[')
		{
			if(state != IN_WHITESPACE)
			{
				cerr<<"Unexpected '[' in search string"<<endl;
				free(word);
				exit(-1);
			}else
			{
				final_search_str += '[';
				state = IN_CLASSIFICATION;
			}
		}else if(character == ']')
		{
			if(state != IN_INFLEXION)
			{
				cerr<<"Unexpected ']' in search string"<<endl;
				free(word);
				exit(-1);
			}else
			{
				uint8_t* normalized_word = (uint8_t*) malloc(word_size);
				size_t normalized_word_size = word_size;	
	
				uint8_t* normalization_result = u8_normalize(UNINORM_NFKD,word,current_word_size,normalized_word, &normalized_word_size);
				if(normalization_result > 0)
				{
					final_search_str += string((const char*) normalized_word, normalized_word_size);
					current_word_size = 0;
				}else
				{
					cerr<<"A word in the search string is longer than the maximum size in this index, and hence no results would be found"<<endl;
					free(normalized_word);
					free(word);
					exit(-1);
				}
				free(normalized_word);

				if(!finished)
					final_search_str += "] ";
				else
					final_search_str += ']';
				state = IN_WHITESPACE;
			}
		}else
		{
			if(state == IN_WHITESPACE)
			{
				state = IN_SINGLE_WORD;
			}
			if(uc_is_property_alphabetic(character))
			{
				character = uc_tolower(character);
			}else if(!(current_word_size && (uc_is_property_hyphen(character) ||
						      (uc_is_property_diacritic(character) && !uc_is_general_category(character,UC_MODIFIER_SYMBOL)))
				  ))
			{
				if(!(((state == IN_SINGLE_WORD) && (character == '$'))) && (character != '*'))
					continue;
			}
			int read = u8_uctomb(word + current_word_size, character,word_size - current_word_size);
			if(read < 0)
			{
				cerr<<"A word in the search string is too long, and hence cannot exist in the index"<<endl;
				free(word);
				exit(-1);
			}else
			{
				current_word_size += read;
			}
		}
	}
	free(word);
	search_string_str = final_search_str;
	if(!relevant_metadata.is_pos && strchr(search_string_str.c_str(),'['))
	{
		cerr<<"You entered a POS search string for a non-POS corpus. Exiting"<<endl;
		exit(-1);
	}
	return;
}

/*
 * This function searches through the POS supplement indexes to generate search strings.
 */
static void flesh_partially_known_words(vector<string> &search_strings,string &foldername)
{

	//At first, we open the pos indexes
	string current_filename(foldername + "/pos_supplement_index_c_l_i/0.out");
	searchable_file *c_l_i = new searchable_file(current_filename);
	current_filename = foldername + "/pos_supplement_index_l_i_c/0.out";
	searchable_file *l_i_c = new searchable_file(current_filename);
	current_filename = foldername + "/pos_supplement_index_i_c_l/0.out";
	searchable_file *i_c_l = new searchable_file(current_filename);

	//We do a simple breadth-first expansion of the relevant words...
	while(1)
	{
		int found_some = 0;
		vector<string> next_level_of_search_strings;
		for(auto i = search_strings.begin(); i != search_strings.end(); i++)
		{
			string current_search_string = *i;
			string::iterator current_char = current_search_string.begin();
			while((current_char = std::find(current_char, current_search_string.end(),'[')) != current_search_string.end())
			{
				string::iterator ending_char = std::find(current_char, current_search_string.end(),']');
				string between(current_char + 1, ending_char );

				string::iterator first_line = find(between.begin(),between.end(),'|');
				string::iterator second_line = find(first_line+1, between.end(),'|');

				string classification(between.begin(),first_line);
				string lemma(first_line + 1, second_line);
				string inflexion(second_line + 1, between.end());
				searchable_file* index_used = NULL;

				vector<string> tmp_result;
				if(classification == "*")
				{
					if(lemma == "*")
					{
						if(inflexion == "*")
						{
							current_char++;
							continue;//Too broad of a scope to search...
						}else
						{
					   		string search_string = inflexion + " ";
					 		string filter = search_string + "* *\t";
							index_used = i_c_l;
							i_c_l->search(search_string,tmp_result,filter);
						}
					}else
					{
						if(inflexion == "*")
						{
							string search_string = lemma + " ";
							string filter = search_string + "* *\t";
							index_used = l_i_c;
							l_i_c->search(search_string,tmp_result,filter);
						}else
						{

							string search_string = lemma + " " +  inflexion + " "; 
							string filter = search_string + "*\t";
							index_used = l_i_c;
							l_i_c->search(search_string,tmp_result,filter);
						}
					}
				}else if(lemma == "*")
				{
					if(inflexion == "*")
					{
						current_char++;
						continue; //Still too broad of a scope to search
					}else
					{
						string search_string = inflexion + " " +  classification + " ";
						string filter = search_string + "*\t";
						index_used = i_c_l;
						i_c_l->search(search_string,tmp_result,filter);
					}
				}else if(inflexion == "*")
				{
					string search_string = classification + " " + lemma + " ";
					string filter = search_string + "*\t";
					index_used = c_l_i;
					c_l_i->search(search_string,tmp_result,filter);
				}

				if(index_used)
				{
					found_some = 1;
					//TODO: optimize the following:
					for(size_t j = 0; j < tmp_result.size(); j++)
					{
						string final_string("");
						stringstream current_string(tmp_result[j]);
						string first,second,third;	
						getline(current_string,first,' ');
						getline(current_string,second,' ');
						getline(current_string,third,'\t');
						if(index_used == c_l_i)
						{
							final_string = "[" + first + "|" + second + "|" + third + "]";
						}else if(index_used == l_i_c)
						{
							final_string = "[" + third + "|" + first + "|" + second + "]";
						}else//i_c_l
						{
							final_string = "[" + second + "|" + third + "|" + first + "]";
						}
						string final_search_string = 
							string(current_search_string.begin(), current_char) +
						       	final_string + 
							string(find(current_char, current_search_string.end(), ']')+ 1,
									current_search_string.end()
							      );
						next_level_of_search_strings.push_back(final_search_string);
					}
					break;
				}
				if(current_char != current_search_string.end())
					current_char++;
				else
				{
					next_level_of_search_strings.push_back(current_search_string);
				}
			}
		}
		if(!found_some)
			break;
		else
			search_strings = next_level_of_search_strings;
	}



	delete c_l_i;
	delete l_i_c;
	delete i_c_l;
}



static vector<pair<vector<string>, long long int> > internal_search(map<unsigned int, Metadata> &metadatas,vector<string> arguments)
{
	if(!arguments.size())
	{
		cerr<<"search: Missing argument, please enter an argument like \"foo * bar baz\" as a search string"<<endl;
		exit(-1);
	}
	if(arguments.size() > 1)
	{
		cerr<<"search: Too many arguments."<<endl;
		exit(-1);
	}

	//We know that all of the metadatas are for the same file, and that there is at least one metadata.
	bool is_pos = (metadatas.begin()->second).is_pos;

	string original_search_string = arguments[0];

	normalize_and_check_search_string(original_search_string,metadatas.begin()->second);
	cerr<<"Searching using normalized search string \""<<original_search_string<<"\""<<endl;

	unsigned int ngramsize = 0;
	unsigned int original_ngramsize = 0;

	//Vectors of word positions for words that we know, words that we partially know and words that are null:
	vector<unsigned int> known_words;
	vector<unsigned int> partially_known_words;
	vector<unsigned int> null_words;
	vector<string> tokenized_search_string;

	//We tokenize the search string by spaces, and initialize the four variables above.
	stringstream argumentstream(original_search_string);
	string cur("");
	while(getline(argumentstream,cur,' '))
	{

		if(cur == "")
			continue;

		if(!is_pos)
		{
			if(cur != "*")
			{
				if(cur == "$")
					null_words.push_back(ngramsize);
				else
					known_words.push_back(ngramsize);
			}
		}else
		{
			if(strchr(cur.c_str(), '*'))
			{
				//Matches *|*]
		    		if((cur.substr(cur.length()-4,3) == "*|*" ))
				{
					if(cur.substr(1,1) != "*")
						partially_known_words.push_back(ngramsize);
				}else 
				{
					known_words.push_back(ngramsize);
				}
			}else
			{
				if(cur == "$")
				{
					null_words.push_back(ngramsize);
				}else
					known_words.push_back(ngramsize);
			}
		}
		tokenized_search_string.push_back(cur);						
		ngramsize++;
	}
	original_ngramsize = ngramsize;

	//We check if we have metadata for the index of that n-gram size
	auto relevant_metadata_itr = metadatas.lower_bound(ngramsize);
	if(relevant_metadata_itr == metadatas.end())
	{
		cerr<<"search: Unable to find metadata for ngrams of size "<<original_ngramsize<<" or greater"<<endl;
		exit(-1);
	}
	ngramsize = relevant_metadata_itr->first;
	for(size_t i = original_ngramsize; i < ngramsize; i++)
	{
		tokenized_search_string.push_back("*");
	}

	Metadata &relevant_metadata = relevant_metadata_itr->second;
	if(is_pos && !relevant_metadata.pos_supplement_indexes_exist)
	{
		cerr<<"search: No POS supplement indexes seem to exist\n";
		exit(-1);
	}

	//We do a simple and unefficient search for the index that matches with the relevant permutation of known and unknown strings
	vector<unsigned int> search_index_to_use;	
	size_t matches = 0;
	size_t classification_only_matches = 0;

	for(set<vector<unsigned int> >::iterator indexes_itr = relevant_metadata.indices.begin(); indexes_itr != relevant_metadata.indices.end();indexes_itr++)
	{
		size_t counter = 0;
		size_t classification_only_counter = 0;

		for(auto j = null_words.begin(); j != null_words.end(); j++)
		{
			if(*((*indexes_itr).begin()) == *j)
				goto continue_upper_loop;
		}
		if(0)
		continue_upper_loop: continue;

		for(auto j = indexes_itr->begin(); j != indexes_itr->end(); j++)
		{
			if(find(known_words.begin(),known_words.end(),*j) != known_words.end())
				counter++;
			else
				break;
		}
		for(auto j = (*indexes_itr).begin(); j!=(*indexes_itr).end();j++)
		{
			if(find(partially_known_words.begin()+matches, partially_known_words.end(),*j) != partially_known_words.end())
				classification_only_counter++;
			else
				break;
		}
		if(counter > matches)
		{
			search_index_to_use = *indexes_itr;
			matches = counter;
			classification_only_matches = classification_only_counter;
		}else if((counter == matches) && (classification_only_counter > classification_only_matches))
		{
			search_index_to_use = *indexes_itr;
			classification_only_matches = classification_only_counter;
		}
	}
	if(!matches)
	{
		cerr<<"search: No indexes found that could do this type of search."<<endl;
		exit(-1);
	}
	//We output a little bit of info before we start the search
	cerr<<"search: Searching with "<<matches<<" matches using index by";
	for_each(search_index_to_use.begin(), search_index_to_use.end(),
			[](const unsigned int &i) ->void
			{
			cerr<<"_"<<i;
			}
		);
	cerr<<endl;

	//We build a search string in the order that the index is in.
	vector<string> tokenized_search_string_in_order;
	for(size_t i = 0; i< ngramsize; i++)
	{
		tokenized_search_string_in_order.push_back(tokenized_search_string[search_index_to_use[i]]);
	}

	vector<string> all_search_strings;
	string tosearchfor;
	for(size_t i = 0; i<matches; i++)
	{
		tosearchfor += tokenized_search_string_in_order[i];
		if(i < (matches - 1))
			tosearchfor += ' ';
	}
	if(matches == ngramsize)
		tosearchfor += '\t';
	else
		tosearchfor += ' ';

	all_search_strings.push_back(tosearchfor);

	if(is_pos)
	{
		flesh_partially_known_words(all_search_strings,relevant_metadata.output_folder_name);
		for(auto i = all_search_strings.begin(); i != all_search_strings.end(); i++)
		{
			i->erase(remove_if(i->begin(),i->end(), [](char c){return (c == '[') || (c == ']');}), i->end());
		}
	}
	//We make a string which has the filename of the index:
	string index_filename = relevant_metadata.output_folder_name + "/by";
	stringstream index_filename_stream(index_filename);
	index_filename_stream.seekp(0, std::ios::end);
	for(size_t i = 0; i< search_index_to_use.size(); i++)
	{
		index_filename_stream<<"_";
		index_filename_stream<<search_index_to_use[i];
	}
	index_filename_stream<<"/";
	index_filename_stream<<"0.out";
	index_filename= index_filename_stream.str();

	searchable_file* index_file = new searchable_file(index_filename,is_pos);


	vector<string> results;
	for(size_t current_search_string_index = 0; current_search_string_index < all_search_strings.size(); current_search_string_index++)
	{
		//We make a filter
		string tosearchfor = all_search_strings[current_search_string_index];
		string filter = tosearchfor;
		for(size_t i = matches; i < ngramsize; i++)
		{
			filter += tokenized_search_string_in_order[i];
			if(i < (ngramsize - 1))
				filter += ' ';
		}

		filter.erase(remove_if(filter.begin(),filter.end(), [](char c) {return ((c == '[') || (c == ']'));}),filter.end());
		if(*(filter.rbegin()) != '\t')
			filter += '\t';

		//We do the actual search
		index_file->search(tosearchfor, results,filter);
	}
	
		
	vector<pair<vector<string>, long long int> > final_results_unreduced;
	for(size_t i = 0; i < results.size(); i++)
	{
		vector<string> current_ngram_in_order_as_vector;

		//We rearrange all of the strings we've found to be in the order that they are in the text.
		string current_ngram = results[i];
		stringstream current_ngram_stream(current_ngram);
		vector<string> current_ngram_vector;
		stringstream current_word;	
		int c;
		while(1)
		{
			c = current_ngram_stream.get();
			if((c == ' ') || (c == '\t') || (c == EOF))
			{
				current_ngram_vector.push_back(current_word.str());
				current_word.str("");
				current_word.clear();
				if(c == EOF)
					break;
			}else
			{
				current_word<<(char)c;
			}
		}
		for(size_t i=0; i<search_index_to_use.size(); i++)
		{
			for(size_t j = 0; j < search_index_to_use.size(); j++)
			{
				if(search_index_to_use[j] == i)
				{
					current_ngram_in_order_as_vector.push_back(current_ngram_vector[j]);
					break;
				}
			}
		}
		const char* startptr = current_ngram_vector[ngramsize].c_str();
		char* endptr;
		long long int number = strtol(startptr,&endptr,10);
		if((startptr != endptr) && (*endptr != '\n'))
		{
			final_results_unreduced.push_back(pair<vector<string>,long long int>(current_ngram_in_order_as_vector,number));
		}else
		{
			cerr<<"Received invalid result"<<endl;
			exit(-1);
		}
	}

	//We sort the unreduced result by the n-gram part.
	sort(final_results_unreduced.begin(),final_results_unreduced.end(),
			[](const pair<vector<string>,long long int> &first, const pair<vector<string>,long long int> &second)
			{
				vector<string> first_ngram = first.first;	
				vector<string> second_ngram = second.first;
				auto i = first_ngram.begin();
				auto j = second_ngram.begin();
				int res = 0;
				for(; i != first_ngram.end(); i++, j++)
				{
					res = i->compare(*j);
					if(res)
					{
						return res < 0;
					}
				}
				return res < 0;
			}
	    );

	vector<pair<vector<string>, long long int> > final_results_reduced;

	if(final_results_unreduced.begin() != final_results_unreduced.end())
	{
		pair<vector<string>, long long int> current_reduced_result;
		for(size_t i = 0; i < original_ngramsize; i++)
		{
			current_reduced_result.first.push_back(final_results_unreduced.begin()->first[i]);
		}
		current_reduced_result.second = final_results_unreduced.begin()->second;
	
		for_each(final_results_unreduced.begin() + 1, final_results_unreduced.end(),
				[&current_reduced_result,&original_ngramsize,&final_results_reduced](const pair<vector<string>,long long int> &cur)
				{
					unsigned int num_matches = 
					[](const vector<string> &shorter_ngram, const vector<string> &second_ngram)->unsigned int
					{
						auto i =  shorter_ngram.begin();
						auto j = second_ngram.begin();
						unsigned int result = 0;
						for(; i != shorter_ngram.end(); i++,j++)
						{
							if(!i->compare(*j))
								result++;
							else
								break;
						}
						return result;
					}(current_reduced_result.first,cur.first);

					if(num_matches >= original_ngramsize)
					{
						current_reduced_result.second += cur.second;
					}else
					{
						final_results_reduced.push_back(current_reduced_result);
						for(size_t i = 0; i<original_ngramsize; i++)
						{
							current_reduced_result.first[i] = cur.first[i];
						}
						current_reduced_result.second = cur.second;
					}

				}
				);
		final_results_reduced.push_back(current_reduced_result);
	}

	delete index_file;
	return final_results_reduced;		
}
void do_search(map<unsigned int,Metadata> &metadatas,vector<string> arguments)
{
	vector<pair<vector<string>, long long int> > results = internal_search(metadatas,arguments);
	sort(results.begin(),results.end(),
			[](const pair<vector<string>,long long int> &first,const pair<vector<string>, long long int> &second)->bool
			{
				return second.second > first.second;
			}
		    );
	for_each(results.begin(),results.end(), 
			[](const pair<vector<string>,long long int> &cur) -> void
			{
				for(auto i = cur.first.begin(); i != cur.first.end(); i++)
				{
					if(i != cur.first.begin())
						cout<<" ";
					cout<<*i;
				}
				cout<<"\t"<<cur.second<<"\n";
			}
		);
}

