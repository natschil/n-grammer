#include "manage_metadata.h"
using namespace std;



//Returns 1 on success
//0 on failure
Metadata::Metadata(string &filename,string &foldername)
{
	num_words = 0;
	time_taken = 0;
	max_frequency = 0;
	this->MAX_LEMMA_SIZE = 40;
	this->MAX_WORD_SIZE = 40; 
	this->MAX_CLASSIFICATION_SIZE = 40;

	folder_name = foldername;
	metadata_filename = foldername + "/" + filename;
	wordlength_stats_exist = 0;

	ifstream metadata_file(metadata_filename.c_str(),ios::in);
	if(!metadata_file)
	{
		cerr<<"File "<<metadata_filename<<" does not exists"<<endl;
		throw 0;
	}
	string nextline;
upper_loop: while(getline(metadata_file,nextline,':'))
	{
		metadata_file.get();

		if(nextline ==	"Numwords")
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
			metadata_file>>MAX_WORD_SIZE;
		}else if(nextline == "MAX_CLASSIFICATION_SIZE")
		{
			metadata_file>>MAX_CLASSIFICATION_SIZE;
		}else if(nextline == "MAX_LEMMA_SIZE")
		{
			metadata_file>>MAX_LEMMA_SIZE;
		}



		metadata_file.get();//To get rid of newline character.
	}

}

void Metadata::write(void)
{
	ofstream outfile(metadata_filename.c_str());	
	outfile<<"Filename:\t"<<file_name<<"\n";
	outfile<<"Numwords:\t"<<num_words<<"\n";
	outfile<<"Time:\t"<<time_taken<<"\n";
	outfile<<"MaxFrequency:\t"<<max_frequency<<"\n";
	outfile<<"MAX_WORD_SIZE:\t"<<MAX_WORD_SIZE<<"\n";
	outfile<<"MAX_CLASSIFICATION_SIZE:\t"<<MAX_CLASSIFICATION_SIZE<<"\n";
	outfile<<"MAX_LEMMA_SIZE:\t"<<MAX_CLASSIFICATION_SIZE<<"\n";
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
	return;
}
