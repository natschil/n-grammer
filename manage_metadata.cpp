#include "manage_metadata.h"
using namespace std;


const char* header_str ="N-Gram-Counter-Version:\t";

//Returns 1 on success
//0 on failure
Metadata::Metadata(string &filename,string &foldername,bool fileMayNotExist)
{
	num_words = 0;
	time_taken = 0;
	max_frequency = 0;
	this->max_word_size = 40; 
	this->max_lemma_size = 40;
	this->max_classification_size = 40;
	this->posIndexesExist = false;
	this->isPos = false;
	this->wordlength_stats_exist = 0;
	this->folder_name = foldername;
	this->metadata_filename = foldername + "/" + filename;
	this->fileExistsAlready = true;

	ifstream metadata_file(metadata_filename.c_str(),ios::in);
	if(!metadata_file)
	{
		this->fileExistsAlready = false;
		if(!fileMayNotExist)
		{
			cerr<<"File "<<metadata_filename<<" does not exists"<<endl;
			throw 0;
		}else
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
			if(!version_number)
			{

				this->fileExistsAlready = false;
				if(!fileMayNotExist)
				{
					cerr<<"Could not read valid version number from file"<<metadata_filename<<endl;
					throw 0;
				}else
					return;
			}
			if(version_number != NGRAM_COUNTER_VERSION)
			{
				this->fileExistsAlready = false;
				if(!fileMayNotExist)
				{
					cerr<<"Version number of file "<<metadata_filename
						<<" differs from version number of this program, ignoring it"
						<<endl;
					throw 0;
				}else
					return;
			}

		}else
		{
			this->fileExistsAlready = false;

			if(!fileMayNotExist)
			{
				cerr<<"The file "<<metadata_filename<<"seems to exist but is invalid metadata"<<endl;
				throw 0;
			}else
				return;
		}
	}else //The file was empty
	{
		this->fileExistsAlready = false;
		if(!fileMayNotExist)
		{
			cerr<<"File "<<metadata_filename<<" is empty"<<endl;
			throw 0;
		}else
			return;	
	}
	string nextline;
upper_loop: while(getline(metadata_file,nextline,':'))
	{
		metadata_file.get();
		if(nextline == "Version")
		{
			int version;
			metadata_file >> version;
			if(version != NGRAM_COUNTER_VERSION)
			{
				cerr<<"The output file was generated with version %d, but this analysis program can only read files\
					of version %d. \n";
			}
		} else if(nextline == "Numwords")
		{
			metadata_file >> num_words;
		}else if(nextline == "Time")
		{
			metadata_file >> time_taken;
		}else if((nextline == "Indexes") || (nextline == "InvertedIndexes"))
		{
			int is_inverted = nextline == "InvertedIndexes" ? 1 : 0;
			while(1)
			{
				int c = metadata_file.get();
				if(c == EOF) 
					break;

				if(((char) c) != '\t')
				{
					metadata_file.unget();
					goto upper_loop;
				}else
				{
					getline(metadata_file,nextline);

					vector<unsigned int> this_combination;

					string index_fname;
					if(!is_inverted)
				        	index_fname	=  nextline.substr(strlen("by"));
					else 
						index_fname = nextline.substr(strlen("inverted_by"));

					stringstream ss(index_fname);
					ss.get();
					while(1)
					{
						unsigned int j = 0;
						ss>>j;
						this_combination.push_back(j);
						ss.get();
						if(ss.eof())
							break;
					}
					if(!is_inverted)
						indices.insert(this_combination);
					else
						inverted_indices.insert(this_combination);
				}
			}
		}else if(nextline == "MaxFrequency")
		{
			metadata_file >> max_frequency;
		}else if(nextline == "Filename")
		{
			getline(metadata_file,file_name);
			metadata_file.unget();
		}else if(nextline == "WordlengthStatsExist")
		{
			string wordlength_stats_exist_str;
			getline(metadata_file,wordlength_stats_exist_str);
			if(wordlength_stats_exist_str == "yes")
			{
				wordlength_stats_exist = true;
			}
			metadata_file.unget();
		}else if(nextline == "MAX_WORD_SIZE")
		{
			metadata_file>>max_word_size;
		}else if(nextline == "MAX_CLASSIFICATION_SIZE")
		{
			metadata_file>>max_classification_size;
		}else if(nextline == "MAX_LEMMA_SIZE")
		{
			metadata_file>>max_lemma_size;
		}else if(nextline == "POSIndexes")
		{
			string pos_indexes_exist_str;
			getline(metadata_file,pos_indexes_exist_str);
			if(pos_indexes_exist_str == "yes")
			{
				posIndexesExist = true;
			}
			metadata_file.unget();//Because we swallowed the newline character which the line below was supposed to swallow
		}else if(nextline == "isPos")
		{
			string is_pos_str;
			getline(metadata_file,is_pos_str);
			if(is_pos_str == "yes")
			{
				isPos = true;
			}
			metadata_file.unget();//Because we swallowed the newline character which the line below was supposed to swallow
		}



		metadata_file.get();//To get rid of newline character.
	}

}

void Metadata::write(void)
{
	ofstream outfile(metadata_filename.c_str());	
	outfile<<header_str<<NGRAM_COUNTER_VERSION<<"\n";
	outfile<<"Filename:\t"<<file_name<<"\n";
	outfile<<"Numwords:\t"<<num_words<<"\n";
	outfile<<"Time:\t"<<time_taken<<"\n";
	if(max_frequency)
		outfile<<"MaxFrequency:\t"<<max_frequency<<"\n";
	outfile<<"MAX_WORD_SIZE:\t"<<max_word_size<<"\n";
	outfile<<"MAX_CLASSIFICATION_SIZE:\t"<<max_classification_size<<"\n";
	outfile<<"MAX_LEMMA_SIZE:\t"<<max_lemma_size<<"\n";
	if(wordlength_stats_exist)
		outfile<<"WordlengthStatsExist:\tyes\n";
	outfile<<"Indexes:"<<"\n";
	for( set<vector<unsigned int> >::iterator i = indices.begin(); i != indices.end(); i++)
	{
		outfile<<"\tby";
		for(vector<unsigned int>::const_iterator j = (*i).begin(); j != (*i).end(); j++)
		{
			outfile<<"_"<<(*j);
		}
		outfile<<"\n";
	}
	if(inverted_indices.begin() != inverted_indices.end())
		outfile<<"InvertedIndexes:\n";
	for(set<vector<unsigned int> >::iterator i = inverted_indices.begin(); i != inverted_indices.end(); i++)
	{
		outfile<<"\tinverted_by";
		for(vector<unsigned int>::const_iterator j = (*i).begin(); j != (*i).end(); j++)
		{
			outfile<<"_"<<(*j);
		}
		outfile<<"\n";
	}
	if(posIndexesExist)
		outfile<<"POSIndexes:\tyes\n";
	if(isPos)
		outfile<<"isPos:\tyes\n";
	return;
}
