#include "manage_metadata.h"
using namespace std;



//Returns 1 on success
//0 on failure
Metadata::Metadata(string &filename,string &foldername)
{
	num_words = 0;
	time_taken = 0;
	max_frequency = 0;
	folder_name = foldername;

	string total_filename = foldername + "/" + filename;

	ifstream metadata_file(total_filename.c_str(),ios::in);
	if(!metadata_file)
	{
		cerr<<"File "<<total_filename<<" does not exists"<<endl;
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
		}else if(nextline == "Indexes")
		{
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

					vector<unsigned int> this_combination(2);
					string index_fname =  nextline.substr(strlen("\tby"));
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
					indices.push_back(this_combination);
				}
			}
		}else if(nextline == "MaxFrequency")
		{
			metadata_file >> max_frequency;
		}else if(nextline == "Filename")
		{
			metadata_file >> file_name;
		}

		metadata_file.get();//To get rid of newline character.
	}

}
