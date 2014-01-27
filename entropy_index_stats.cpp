#include "entropy_index_stats.h"

void  entropy_index_stats(map<unsigned int, Metadata> &metadatas, vector<string> arguments)
{
	if(arguments.size() != 1)
	{
		cerr<<"Please give an argument"<<endl;
		exit(-1);
	}

	unsigned int ngramsize = split_ignoring_empty(arguments[0],' ').size();
	auto relevant_metadata_itr = metadatas.find(ngramsize);
	if(relevant_metadata_itr == metadatas.end())
	{
		cerr<<"No metadata for n-grams of size "<<ngramsize<<" found"<<endl;
		exit(-1);
	}
	auto &metadata = relevant_metadata_itr->second;
	string index_location{""};
	if((index_location = metadata.get_entropy_index(arguments[0])) == "")
	{
		cerr<<"Could not entropy indexes for specified pattern"<<endl;
		exit(-1);
	}
	string index_location_full_path = metadata.output_folder_name + "/" + index_location;

	FILE* f =  fopen(index_location_full_path.c_str(),"r");
	if(!f)
	{
		cerr<<"Could not open file "<<index_location_full_path<<endl;
		exit(-1);
	}

	Metadata::skip_entropy_index_header(f,index_location_full_path);
	//Read a float and a uint_64_t in succession.
	map<int,unsigned int> frequencies{};
	int granularity = 1000;
	while(true)
	{
		float current_entropy;
		uint64_t current_offset;

		if(fread(&current_entropy,sizeof(current_entropy),1,f) != 1)
		{
			cerr<<"Reached end of file, exiting (but with zero exit status)"<<endl;
			break;
		}
		if(fread(&current_offset,sizeof(current_offset),1,f) != 1)
		{
			cerr<<"Reached premature eof, exiting (with non-zero exit status)"<<endl;	
			exit(-1);
		}

		int key = round(current_entropy * granularity);
		auto iterator = frequencies.insert(pair<int,unsigned int>{key,1});
		if(iterator.second == false)
			(iterator.first->second)++;
	}
	fclose(f);
	for(const auto &itr :frequencies)
	{
		cout<<(((float)(itr.first))/granularity)<<"\t"<<(itr.second)<<endl;
	}
}
