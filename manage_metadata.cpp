#include "manage_metadata.h"
using namespace std;



//Returns 1 on success
//0 on failure
Metadata::Metadata(string filename)
{
	num_words = 0;
	time_taken = 0;
	max_frequency = 0;

	ifstream metadata_file(filename.c_str(),ios::in);
	if(!metadata_file)
	{
		cerr<<"File "<<filename<<" does not exists"<<endl;
		throw 0;
	}
	string nextline;
	while(getline(metadata_file,nextline,':'))
	{
loop_start:
		if(nextline ==	"Numwords")
		{
			metadata_file.get();
			metadata_file >> num_words;
		}else if(nextline == "Time")
		{
			metadata_file.get();
			metadata_file >> time_taken;
		}else if(nextline == "Indexes")
		{
			while(1)
			{
				metadata_file.get(); //Get rid of the space
				getline(metadata_file,nextline);
				if(nextline[0] != '\t')
					goto loop_start;
				else
				{
					vector<unsigned int> this_combination(2);
					string index_fname =  nextline.substr(strlen("\tby"));
					stringstream ss(nextline);
					while(1)
					{
						ss.get();//Ignore the '_'
						unsigned int j = 0;
						ss<<j;
						this_combination.push_back(j);
						if(ss.eof())
							break;
					}
					indices.push_back(this_combination);
				}
			}
		}else if(nextline == "MaxFrequency")
		{
			metadata_file.get();
			metadata_file >> max_frequency;
		}else if(nextline == "Filename")
		{
			metadata_file.get();
			metadata_file >> file_name;
		}

		metadata_file.get();//To get rid of newline character.
	}

	throw 1;
}
