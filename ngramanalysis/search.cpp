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
					if(current_word_size && ((word[0] == '$') || (word[0] == '*')) && (current_word_size != 1))
					{
						cerr<<"search: '$' or '*' may not be part of a word"<<endl;
						free(word);
						exit(-1);
					}


					if(relevant_metadata.is_pos && (word[0] != '$' ))
						final_search_str += "[*|*|";

					uint8_t* normalized_word = (uint8_t*) malloc(word_size);
					size_t normalized_word_size = word_size;	
	
					uint8_t* normalization_result = u8_normalize(UNINORM_NFKD,word,current_word_size,normalized_word, &normalized_word_size);
					if(normalization_result)
					{
						final_search_str += string((const char*) normalized_word, normalized_word_size);
						free(normalized_word);
						current_word_size = 0;
					}else
					{
						cerr<<"search: A word in the search string is longer than the maximum size in this index, and hence no results would be found"<<endl;
						free(normalized_word);
						free(word);
						exit(-1);
					}

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
			}
			if(finished)
				break;
		}else if(character == '|')
		{
			switch(state)
			{
			case IN_WHITESPACE: 	
						if(relevant_metadata.is_pos)
							cerr<<"Search string has invalid format, expected '[' instead of '|'"<<endl;
						else
							cerr<<"Search string contains illegal character '|'. (It's a non-POS corpus)"
								<<endl;
					    	free(word);
					    	exit(-1);
			break;
			case IN_CLASSIFICATION: state = IN_LEMMA; 
			break;
			case IN_LEMMA: 		state = IN_INFLEXION; 
			break;
			case IN_INFLEXION: 	
						if(relevant_metadata.is_pos)
							cerr<<"Search string has invalid format, expected ']' instead of '|'"<<endl;
						else
							cerr<<"Search string contains illegal characeter '|'. (It's a non-POS corpus"<<endl;
					   	free(word);
					   	exit(-1);
			break;
			case IN_SINGLE_WORD:	
						if(relevant_metadata.is_pos)
							cerr<<"Search string has invalid format, expected ' ' instead of '|'"<<endl;
						else
							cerr<<"Search string contains illegal character '|'. (It's a non-POS corpus)"<<endl;
						free(word);
						exit(-1);
			}
			uint8_t* normalized_word = (uint8_t*) malloc(word_size);
			size_t normalized_word_size = word_size;	

			uint8_t* normalization_result = u8_normalize(UNINORM_NFKD,word,current_word_size,normalized_word, &normalized_word_size);
			if(normalization_result)
			{
				final_search_str += string((const char*) normalized_word, normalized_word_size);
				free(normalized_word);
				current_word_size = 0;
			}else
			{
				cerr<<"A word in the search string is longer than the maximum size in this index, and hence no results would be found"<<endl;
				free(normalized_word);
				free(word);
				exit(-1);
			}

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
				if(relevant_metadata.is_pos)
				{
					final_search_str += '[';
					state = IN_CLASSIFICATION;
				}else
				{
					cerr<<"Search strings contains illegal character '['"<<endl;
					free(word);
					exit(-1);
				}
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
				if(!relevant_metadata.is_pos)
				{
					cerr<<"Unexpected ']' in search string. (It's a non-POS corpus)"<<endl;
					free(word);
					exit(-1);
				}
				uint8_t* normalized_word = (uint8_t*) malloc(word_size);
				size_t normalized_word_size = word_size;	
	
				uint8_t* normalization_result = u8_normalize(UNINORM_NFKD,word,current_word_size,normalized_word, &normalized_word_size);
				if(normalization_result > 0)
				{
					final_search_str += string((const char*) normalized_word, normalized_word_size);
					free(normalized_word);
					current_word_size = 0;
				}else
				{
					cerr<<"A word in the search string is longer than the maximum size in this index, and hence no results would be found"<<endl;
					free(normalized_word);
					free(word);
					exit(-1);
				}

				if(ptr != eof)
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
					   		string search_string = inflexion + "\t";
					 		string filter = search_string + "*\t*\t";
							index_used = i_c_l;
							i_c_l->search(search_string,tmp_result,filter);
						}
					}else
					{
						if(inflexion == "*")
						{
							string search_string = lemma + "\t";
							string filter = search_string + "*\t*\t";
							index_used = l_i_c;
							l_i_c->search(search_string,tmp_result,filter);
						}else
						{

							string search_string = lemma + "\t" +  inflexion + "\t"; 
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
						string search_string = inflexion + "\t" +  classification + "\t";
						string filter = search_string + "*\t";
						index_used = i_c_l;
						i_c_l->search(search_string,tmp_result,filter);
					}
				}else if(inflexion == "*")
				{
					string search_string = classification + "\t" + lemma + "\t";
					string filter = search_string + "*\t";
					index_used = c_l_i;
					c_l_i->search(search_string,tmp_result,filter);
				}

				if(index_used)
				{
					if(tmp_result.empty())
					{
						cerr<<"search: Warning - at least one of the words used in this string doesn't seem to exist *at all* in the indexes"<<endl;
					}
					found_some = 1;
					//TODO: optimize the following:
					for(size_t j = 0; j < tmp_result.size(); j++)
					{
						string final_string("");
						stringstream current_string(tmp_result[j]);
						string first,second,third;	
						getline(current_string,first,'\t');
						getline(current_string,second,'\t');
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



vector<pair<vector<string>, long long int> > internal_search(map<unsigned int, Metadata> &metadatas,vector<string> arguments)
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
	vector<unsigned int> partially_known_words;
	vector<unsigned int> null_words;
	vector<unsigned int> known_words;
	vector<string> tokenized_search_string = split(original_search_string,' ');

	for(auto &cur : tokenized_search_string)
	{
		if(cur == "")
		{
			cerr<<"This seems to be a bug (empty string in tokenized search string), exiting..."<<endl;
			exit(-1);
		}

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
				//Matches |*|*]
		    		if((cur.length() >= 5) &&(cur.substr(cur.length()-5,5) == "|*|*]" ))
				{
					if(cur.substr(1,1) != "*")
						partially_known_words.push_back(ngramsize);
				}else 
				{
					known_words.push_back(ngramsize); //We treat [*|foo|*] or [*|*|bar] as a known word
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
		ngramsize++;
	}
	original_ngramsize = ngramsize;

	//We check if we have metadata for the index of that n-gram size
	auto relevant_metadata_itr = metadatas.lower_bound(ngramsize);
	vector<unsigned int> search_index_to_use;	
	size_t matches;
	size_t classification_only_matches;
	Metadata *relevant_metadata;


	do{
		if(relevant_metadata_itr == metadatas.end())
		{
			cerr<<"search: Unable to indexes for ngrams of size "<<original_ngramsize<<" or greater that could do this search"<<endl;
			exit(-1);
		}

		search_index_to_use.resize(0);
		matches = 0;
		classification_only_matches = 0;
		relevant_metadata = &relevant_metadata_itr->second;
		tokenized_search_string.resize(original_ngramsize);

		ngramsize = relevant_metadata_itr->first;
		for(size_t i = original_ngramsize; i < ngramsize; i++)
		{
			if(is_pos)
				tokenized_search_string.push_back("[*|*|*]");
			else
				tokenized_search_string.push_back("*");
		}

		if(is_pos && !relevant_metadata->pos_supplement_indexes_exist)
		{
			cerr<<"search: No POS supplement indexes seem to exist\n";
			exit(-1);
		}

		//We do a simple and unefficient search for the index that matches with the relevant permutation of known and unknown strings

		for(set<vector<unsigned int> >::iterator indexes_itr = relevant_metadata->indices.begin(); indexes_itr != relevant_metadata->indices.end();indexes_itr++)
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
		relevant_metadata_itr++;
	}while(!matches);
	//We output a little bit of info before we start the search
	string index_specification = join(functional_map(search_index_to_use,unsigned_int_to_string),"_");
	cerr<<"search: Searching with "<<matches<<" matches using index by_"<<index_specification<<endl;;

	vector<string> all_search_strings{join(slice(permute(tokenized_search_string,search_index_to_use),0,matches),"\t") + '\t'};

	if(is_pos)
	{
		flesh_partially_known_words(all_search_strings,relevant_metadata->output_folder_name);
		//At this point we remove all '[' s and ']'s as they were mainly there for aesthetic reasons anyways
		function<bool(char)> is_square_bracket = [](char c) -> bool{return (c == '[') || (c == ']');};
		for(auto &i :all_search_strings)
		{
			i.erase(remove_if(i.begin(),i.end(), is_square_bracket), i.end());
		}
		for(auto &i : tokenized_search_string)
		{
			i.erase(remove_if(i.begin(),i.end(),is_square_bracket), i.end());
		}
	}
	//We make a string which has the filename of the index:
	string index_filename = relevant_metadata->output_folder_name + "/by_" + index_specification + "/0.out";

	searchable_file* index_file = new searchable_file(index_filename,is_pos);


	vector<string> results;
	string filter_part_two  = join(slice(permute(tokenized_search_string,search_index_to_use),matches,ngramsize),"\t") + "\t";
	for(auto &tosearchfor : all_search_strings)
	{
		//We make a filter
		string filter = tosearchfor + (filter_part_two == "\t" ? string("") :filter_part_two);
		//We do the actual search
		index_file->search(tosearchfor, results,filter);
	}
	
		
	vector<pair<vector<string>, long long int> > final_results_unreduced;
	vector<unsigned int> inverse_permutation;
	for(auto i: range(0u,ngramsize))
	{
		auto res = find(search_index_to_use.begin(), search_index_to_use.end(),i);
		inverse_permutation.push_back(res - search_index_to_use.begin());
	}
	inverse_permutation.push_back(ngramsize);
	for(auto &current_ngram : results)
	{
		vector<string> current_ngram_in_order_as_vector = permute(split(current_ngram,'\t'),inverse_permutation);
		string last_number = current_ngram_in_order_as_vector.back();
		current_ngram_in_order_as_vector.pop_back();
		final_results_unreduced.push_back({move(current_ngram_in_order_as_vector),stoll(last_number)});
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

	/*
	 * At this point, in the case that we searched for n-grams in a (n+k)-gram index, we need to combine entries 
	 * that are equal except for the last k words.
	 */
	vector<pair<vector<string>, long long int> > final_results_reduced;
	if((final_results_unreduced.begin() != final_results_unreduced.end()) && (ngramsize != original_ngramsize))
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
	}else
	{
		final_results_reduced = final_results_unreduced;
	}


	delete index_file;
	return final_results_reduced;		
}
void do_search(map<unsigned int,Metadata> &metadatas,vector<string> arguments)
{
	vector<pair<vector<string>, long long int> > results = internal_search(metadatas,arguments);
	//We return our results sorted by frequency, because it looks nice.
	sort(results.begin(),results.end(),
			[](const pair<vector<string>,long long int> &first,const pair<vector<string>, long long int> &second)->bool
			{
				return second.second < first.second;
			}
		    );
	for( auto & cur: results)
	{
		cout<<join(cur.first,"\t")<<"\t"<<cur.second<<endl;
	}
}

