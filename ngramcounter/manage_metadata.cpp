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
		}else if((nextline == "Indexes") || (nextline == "InvertedIndexes"))
		{
			int is_inverted = ((nextline == "InvertedIndexes") ? 1 : 0);
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

					string index_as_string;
					if(!is_inverted)
				        	index_as_string=  nextline.substr(strlen("by_"));
					else 
						index_as_string = nextline.substr(strlen("inverted_by_"));

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
					if(!is_inverted)
						indices.insert(this_combination);
					else
						inverted_indices.insert(this_combination);
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
	if(this->wordlength_stats_exist)
		outfile<<"WordlengthStatsExist:\tyes\n";
	return;
}
