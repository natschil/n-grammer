#include "manage_metadata.h"
using namespace std;

//Every metadata file should start with something like N-Gram-Counter-Version:\t8\n
const char* header_str ="N-Gram-Counter-Version:\t";

Metadata::Metadata(string &metadata_filename)
{
	this->file_name = string("");
	const char* ptr = strrchr(metadata_filename.c_str(),'/');
	if(!ptr)
	{
		this->output_folder_name = string("./");
	}else
	{
		this->output_folder_name = string(metadata_filename.c_str(), ptr + 1);
	}
	this->num_words = 0;
	this->is_pos = false;

	this->time_taken = -1;

	this->max_word_size = 40; 
	this->max_classification_size = 40;
	this->max_lemma_size = 40;

	this->pos_supplement_indexes_exist = false;
	
	this->metadata_filename = metadata_filename;
	this->file_exists_already = true; //Assume it is valid until we find out otherwise.

	this->wordlength_stats_exist = 0;



	ifstream metadata_file(metadata_filename.c_str(),ios::in);
	if(!metadata_file)
	{
		this->file_exists_already = false;
		return;
	}
	string firstline;
	if(getline(metadata_file,firstline))
	{
		//The first line should something like: N-Gram-Counter-Version: 2
		if(firstline.substr(0,strlen(header_str)) == string(header_str))
		{
			int version_number = 0;
			version_number = atoi(firstline.substr(strlen(header_str), firstline.length() - strlen(header_str)).c_str());
			if(!version_number || (version_number != NGRAM_COUNTER_OUTPUT_FORMAT_VERSION))
			{
				this->file_exists_already = false;
				return;
			}
		}else
		{
			this->file_exists_already = false;
			return;
		}
	}else //The file was empty
	{
		this->file_exists_already = false;
		return;	
	}


	string nextline;
upper_loop: while(getline(metadata_file,nextline,':'))
	{
		//Get rid of the tab (or sometimes newline, in the case of "Indexes:\n") that always follows colons.
		metadata_file.get();
		if(nextline == "Version")
		{
			int version;
			metadata_file >> version;
			if(version != NGRAM_COUNTER_OUTPUT_FORMAT_VERSION)
			{
				cerr<<"The output file was generated with version %d, but this analysis program can only read files\
					of version %d. \n";
			}
		}else if(nextline == "Filename")
		{
			getline(metadata_file,file_name);
			metadata_file.unget();
		}else if(nextline == "Numwords")
		{
			metadata_file >> num_words;
		}else if(nextline == "isPos")
		{
			string is_pos_str;
			getline(metadata_file,is_pos_str);
			if(is_pos_str == "yes")
			{
				is_pos = true;
			}
			metadata_file.unget();//Because we swallowed the newline character which the line below was supposed to swallow
		}else if(nextline == "Time")
		{
			metadata_file >> time_taken;
		}else if(nextline == "MAX_WORD_SIZE")
		{
			metadata_file>>max_word_size;
		}else if(nextline == "MAX_CLASSIFICATION_SIZE")
		{
			metadata_file>>max_classification_size;
		}else if(nextline == "MAX_LEMMA_SIZE")
		{
			metadata_file>>max_lemma_size;
		}else if((nextline == "Indexes") || (nextline == "InvertedIndexes") || (nextline == "EntropyInvertedIndexes"))
		{
			int index_type = ((nextline == "Indexes") ? 0 : ((nextline == "InvertedIndexes") ? 1 : 2));
			while(1)
			{
				//Get the first character in the new line, note that due to the .get() above, this is true also for the first line.
				int c = metadata_file.get();
				if(c == EOF) //If we've reached the end of the file, stop.
					break;

				//Any indexes should start with a tab, else, push back whatever we just read and see what it is.
				if(((char) c) != '\t')
				{
					metadata_file.unget();
					goto upper_loop;
				}else
				{
					//We read in the rest of the line.
					getline(metadata_file,nextline);

					vector<unsigned int> this_combination;

					unsigned int entropy_known_words_number = 0;

					string index_as_string;
					switch(index_type)
					{
						case 0:
							index_as_string = nextline.substr(strlen("by_"));
						break;
						case 1:
							index_as_string = nextline.substr(strlen("inverted_by_"));
						break;
						case 2:
						{
							const char* ptr = nextline.c_str();
							const char* ptr2 = ptr + strlen("entropy_");
							char* endptr;
							entropy_known_words_number = strtoul(ptr2,&endptr,10);
							ptr2 = endptr;
							ptr2 += strlen("_index_");
							index_as_string = nextline.substr(ptr2 - ptr);
						}
					}

					stringstream index_as_stringstream(index_as_string);
					while(1)
					{
						unsigned int j = 0;
						index_as_stringstream>>j;
						this_combination.push_back(j);
						index_as_stringstream.get();//Get rid of the undescroes
						if(index_as_stringstream.eof())
							break;
					}
					switch(index_type)
					{
						case 0:
							indices.insert(this_combination);
						break;
						case 1:
							inverted_indices.insert(this_combination);
						break;
						case 2:
						{
							pair<unsigned int,vector<unsigned int> > value(entropy_known_words_number, this_combination);
							entropy_indexes.insert(value);
						}
						break;
							
					}
				}
			}
		}else if(nextline == "POSSupplementIndexesExist")
		{
			string pos_supplement_indexes_exist_str;
			getline(metadata_file,pos_supplement_indexes_exist_str);
			if(pos_supplement_indexes_exist_str == "yes")
			{
				pos_supplement_indexes_exist = true;
			}
			metadata_file.unget();//Because we swallowed the newline character in getline() which the line below was supposed to swallow
		}else if(nextline == "WordlengthStatsExist")
		{
			string wordlength_stats_exist_str;
			getline(metadata_file,wordlength_stats_exist_str);
			if(wordlength_stats_exist_str == "yes")
			{
				wordlength_stats_exist = true;
			}
			metadata_file.unget();
		}

		metadata_file.get();//To get rid of the trailing newline character.
	}

}

void Metadata::write(void)
{
	ofstream outfile(metadata_filename.c_str());	
	outfile<<header_str<<NGRAM_COUNTER_OUTPUT_FORMAT_VERSION<<"\n";

	outfile<<"Filename:\t"<<this->file_name<<"\n";
	outfile<<"Numwords:\t"<<this->num_words<<"\n";
	if(this->is_pos)
		outfile<<"isPos:\tyes\n";

	outfile<<"Time:\t"<<this->time_taken<<"\n";
	outfile<<"MAX_WORD_SIZE:\t"<<this->max_word_size<<"\n";
	outfile<<"MAX_CLASSIFICATION_SIZE:\t"<<this->max_classification_size<<"\n";
	outfile<<"MAX_LEMMA_SIZE:\t"<<this->max_lemma_size<<"\n";

	if(this->pos_supplement_indexes_exist)
		outfile<<"POSSupplementIndexesExist:\tyes\n";

	outfile<<"Indexes:"<<"\n";
	for( auto i = this->indices.begin(); i != this->indices.end(); i++)
	{
		outfile<<"\tby";
		for(auto j = (*i).begin(); j != (*i).end(); j++)
		{
			outfile<<"_"<<(*j);
		}
		outfile<<"\n";
	}

	if(this->inverted_indices.begin() != this->inverted_indices.end())
		outfile<<"InvertedIndexes:\n";

	for(auto i = this->inverted_indices.begin(); i != this->inverted_indices.end(); i++)
	{
		outfile<<"\tinverted_by";
		for(auto j = (*i).begin(); j != (*i).end(); j++)
		{
			outfile<<"_"<<(*j);
		}
		outfile<<"\n";
	}

	if(this->entropy_indexes.begin() != this->entropy_indexes.end())
	{
		outfile<<"EntropyInvertedIndexes:\n";
	}

	for(auto i = this->entropy_indexes.begin(); i != this->entropy_indexes.end(); i++)
	{
		outfile<<"\tentropy_"<<i->first<<"_index";
		for(auto j = i->second.begin(); j != i->second.end(); j++)
		{
			outfile<<"_"<<*j;
		}
		outfile<<"\n";
	}
	if(this->wordlength_stats_exist)
		outfile<<"WordlengthStatsExist:\tyes\n";
	return;
}


string Metadata::get_entropy_index(string pattern)
{
	string index_to_use{""};
	vector<string> tokenized_string = split_ignoring_empty(pattern,' ');
	vector<unsigned int> known_words;
	vector<unsigned int> unknown_words;
	for(size_t i = 0; i< tokenized_string.size(); i++)
	{
		string &cur = tokenized_string[i];
		if(cur.length() != 1)
		{
			cerr<<"Token "<<cur<<" is too long"<<endl;
			return "";
		}
		switch(cur[0])
		{
			case '*': unknown_words.push_back(i);
			break;
			case '=': known_words.push_back(i);
		}
	}
	pair<unsigned int,vector<unsigned int> > search_index_to_use;
	unsigned int matches = 0;
	for(auto i = this->entropy_indexes.begin(); i != this->entropy_indexes.end(); i++)
	{
		if(i->first != known_words.size())
			continue;
		unsigned int current_matches = 0;	
		for(auto j = i->second.begin(); j != i->second.end(); j++)
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
		cerr<<"Could not find a any entropy indexes for this search string.\n"<<endl;
		return "";
	}

	string index_specifier = join(functional_map(search_index_to_use.second,unsigned_int_to_string),"_");
	return "entropy_" + to_string(known_words.size()) + "_index_" +  index_specifier;
}

void Metadata::skip_entropy_index_header(FILE* entropy_index_file,string entropy_index_filename)
{
	size_t header_line_size = 1024;
	char* header_line = (char*)malloc(header_line_size);
	if(getline(&header_line,&header_line_size,entropy_index_file) <= 0)
	{
		cerr<<"Could not read enough bytes from file "<<entropy_index_filename<<endl;
		exit(-1);
	}
	if(strcmp(header_line,"Entropy Index File in Binary Format\n"))
	{
		cerr<<"Entropy index file "<<entropy_index_filename<<" has incorrect header part one."<<endl;
		exit(-1);
	}
	if(getline(&header_line,&header_line_size,entropy_index_file) <= 0)
	{
		cerr<<"Could not read enough bytes from file "<<entropy_index_filename<<endl;
		exit(-1);
	}
	if(strcmp(header_line,"---Begin Binary Reference Info---\n"))
	{
		cerr<<"Entropy index file "<<entropy_index_filename<<" has incorrect header part two."<<endl;
		exit(-1);
	}
	float float_to_test;
	uint64_t uint_to_test;
	if(fread(&float_to_test,sizeof(float_to_test),1,entropy_index_file) != 1)
	{
		cerr<<"Could not read reference float from entropy index file "<<entropy_index_filename<<endl;
		exit(-1);
	}
	if(fread(&uint_to_test,sizeof(uint_to_test),1,entropy_index_file) != 1)
	{
		cerr<<"Could not read reference uint from entropy index file "<<entropy_index_filename<<endl;
		exit(-1);
	}
	if((float_to_test != 123.125) || (uint_to_test != 1234567890))
	{
		cerr<<"It seems like the entropy indexes were not generated on this machine, and this machine uses a different binary format"<<endl;
		exit(-1);
	}
	if(getc(entropy_index_file) != '\n')
	{
		cerr<<"Expected newline in entropy index file, did not find it, exiting"<<endl;
		exit(-1);
	}
	if(getline(&header_line,&header_line_size,entropy_index_file) <= 0)
	{
		cerr<<"Could not read enough bytes from file "<<entropy_index_filename<<endl;
		exit(-1);
	}
	if(strcmp(header_line,"---End Binary Reference Info---\n"))
	{
		cerr<<"Entropy index file "<<entropy_index_filename<<" has incorrect header part three."<<endl;
		exit(-1);
	}

	if(getline(&header_line,&header_line_size,entropy_index_file) <= 0)
	{
		cerr<<"Could not read enough bytes from file "<<entropy_index_filename<<endl;
		exit(-1);
	}
	if(strcmp(header_line,"---Actual Index is in Binary Form Below---\n"))
	{
		cerr<<"Entropy index file "<<entropy_index_filename<<" has incorrect header part four."<<endl;
		exit(-1);
	}
	free(header_line);
	return;
}
