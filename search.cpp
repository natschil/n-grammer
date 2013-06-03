#include "search.h"

using namespace std;

//Replace any occurances of word with [word||] and any occurance of * not within square brackets with [*|*|]
static void normalize_search_string_format(string& search_string_str)
{
	const char* search_string = search_string_str.c_str();
	string new_search_string("");

	const char* ptr;
	bool in_brackets = false;
	string current_inflexion("");
	string current_classification("");
	string current_lemma("");

	string* current_string_part = &current_inflexion;

	for(ptr = search_string; ; ptr++)
	{
		if(*ptr == ' '|| !*ptr)
		{
			if(in_brackets)
			{
				cerr<<"search: No spaces allowed within word descriptors for POS searches"<<endl;
				exit(-1);
			}else
			{
				if((ptr > search_string) && (*(ptr -1 ) != ']'))
				{
					new_search_string += ("[*|*|" + *current_string_part + "]");
				}
				if(*ptr)
				{
					new_search_string += ' ';
				}
				current_inflexion = "";
				current_classification ="";
				current_lemma="";
				current_string_part = &current_inflexion;
			}
			if(!*ptr)
				break;
		}else if(*ptr == '[')
		{
			if(in_brackets)
			{
				cerr<<"search: No nesting of brackets allowd in search string"<<endl;
				exit(-1);
			}else
			{
				in_brackets = true;
				current_inflexion = "";
				current_classification ="";
				current_lemma="";
				current_string_part = &current_inflexion;
				new_search_string += "[";
			}
		}else if(*ptr == ']')
		{
			if(!in_brackets)
			{
				cerr<<"search: Mismatched brackets in search string"<<endl;
				exit(-1);
			}else
			{
				if(current_string_part != &current_lemma)
				{
					cerr<<"search: Expected one more '|'"<<endl;
					exit(-1);
				}
				if(current_lemma == "")
				{
					cerr<<"search:Empty search string part. Perhaps you mean '*'?"<<endl;
					exit(-1);
				}
				new_search_string += current_lemma;
				in_brackets = false;
				current_inflexion = "";
				current_classification ="";
				current_lemma="";
				current_string_part = &current_inflexion;
				new_search_string += ']';
			}

		}else if(*ptr == '|')
		{
			if(!in_brackets)
			{
				cerr<<"search: No '|' characters allowed outside of brackets"<<endl;
				exit(-1);
			}else
			{
				if(*current_string_part == "")
				{
					cerr<<"search: Found empty search string part. Did you maybe mean '*' instead?"<<endl;
					exit(-1);
				}
				if(current_string_part != &current_lemma)
				{
					new_search_string += *current_string_part;
					if(current_string_part == &current_inflexion)
						current_string_part = &current_classification;
					else
						current_string_part = &current_lemma;

				}else
				{
					cerr<<"search: Found '|' after lemma, Did you mean ']'?"<<endl;
					exit(-1);
				}

				new_search_string += '|';
			}

		}else if(*ptr == '*')
		{
			if((*current_string_part != "") && (!strchr("]| ",*(ptr+1))))
			{
				cerr<<"search: The '*' character must appear on its own in a search string part"<<endl;
				exit(-1);
			}

			*current_string_part = '*';
		}else{
			*current_string_part += *ptr;
		}
	}
	search_string_str = new_search_string;
	return;
}

static void flesh_partially_known_words(vector<string> &search_strings,string &foldername)
{

	//At first, we open the pos indexes
	searchable_file *c_l_i = new searchable_file(foldername + "/pos_supplement_index_c_l_i/0.out");
	searchable_file *l_i_c = new searchable_file(foldername + "/pos_supplement_index_l_i_c/0.out");
	searchable_file *i_c_l = new searchable_file(foldername + "/pos_supplement_index_i_c_l/0.out");

	//We do a simple breadth-first expanding of the relevant words...
	while(1)
	{
		int found_some = 0;
		vector<string> next_level_of_search_strings;
		for(vector<string>::iterator i = search_strings.begin(); i!=search_strings.end(); i++)
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
					 		string filter = search_string + "* *";
							index_used = i_c_l;
							i_c_l->search(search_string,tmp_result,filter);
						}
					}else
					{
						if(inflexion == "*")
						{
							string search_string = lemma + " ";
							string filter = search_string + "* *";
							index_used = l_i_c;
							l_i_c->search(search_string,tmp_result,filter);
						}else
						{

							string search_string = lemma + " " +  inflexion + " "; 
							string filter = search_string + "*";
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
						string filter = search_string + "*";
						index_used = i_c_l;
						i_c_l->search(search_string,tmp_result,filter);
					}
				}else if(inflexion == "*")
				{
					string search_string = classification + " " + lemma + " ";
					string filter = search_string + "*";
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
				current_char++;

			}
		}
		if(!found_some)
		{
			break;
		}else
			search_strings = next_level_of_search_strings;
	}



	delete c_l_i;
	delete l_i_c;
	delete i_c_l;
}




void do_search(map<unsigned int,Metadata> &metadatas,vector<string> arguments)
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
	bool is_pos = (metadatas.begin()->second).isPos;

	string search_string = arguments[0];
	if(is_pos)
	{
		normalize_search_string_format(search_string);
	}

	//word positions that are part of the search string
	size_t ngramsize = 0;
	vector<unsigned int> known_words;
	vector<unsigned int> partially_known_words;
	vector<string> tokenized_search_string;

	//We tokenize the search string by spaces, and initialize the three variables above.
	stringstream argumentstream(search_string);
	string cur("");
	while(getline(argumentstream,cur,' '))
	{

		if(cur == "")
			continue;

		if(!is_pos)
		{
			if(cur != "*")
			{
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
				known_words.push_back(ngramsize);
			}
		}
		tokenized_search_string.push_back(cur);						
		ngramsize++;
	}

	//We check if we have metadata for the index of that n-gram size
	if(metadatas.find(ngramsize) == metadatas.end())
	{
		cerr<<"search: Unable to find metadata for ngrams of size "<<ngramsize<<endl;
		exit(-1);
	}
	Metadata &relevant_metadata = metadatas[ngramsize];
	if(is_pos && !relevant_metadata.posIndexesExist)
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
		vector<unsigned int>::const_iterator j;
		for(j = (*indexes_itr).begin(); j != (*indexes_itr).end(); j++)
		{
			if(find(known_words.begin(),known_words.end(),*j) != known_words.end())
				counter++;
			else
				break;
		}
		for(j = (*indexes_itr).begin(); j!=(*indexes_itr).end();j++)
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

	for(size_t i = 0; i < search_index_to_use.size(); i++)
	{
		cerr<<"_";
		cerr<<search_index_to_use[i];
	}
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
		flesh_partially_known_words(all_search_strings,relevant_metadata.folder_name);
		for(size_t i = 0; i<all_search_strings.size(); i++)
		{
			string &this_search_string = all_search_strings[i];
			this_search_string.erase(remove_if(this_search_string.begin(), this_search_string.end(), [](char c) {return (c == '[') || (c == ']');}),this_search_string.end());

		}
	}
	//We make a string which has the filename of the index:
	string index_filename = relevant_metadata.folder_name + "/by";
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

		//We do the actual search
		index_file->search(tosearchfor, results,filter);
	}
	
		
	for(size_t i = 0; i < results.size(); i++)
	{
		string current_ngram = results[i];

		//We rearrange all of the strings we've found to be in the order that they are in the text.
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
			if(i)
				cout<<" ";
			for(size_t j = 0; j < search_index_to_use.size(); j++)
			{
				if(search_index_to_use[j] == i)
				{
					cout<<current_ngram_vector[j];
					break;
				}
			}
		}
		cout<<"\t"<<current_ngram_vector[ngramsize]<<"\n";
	}

	delete index_file;
	return;		
}
