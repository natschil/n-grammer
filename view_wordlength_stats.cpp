#include "view_wordlength_stats.h"

void view_wordlength_stats(map<unsigned int,Metadata> &metadatas,vector<string> &arguments)
{
	if(!arguments.size())
	{
		cerr<<"view_wordlenth_stats: Missing argument, please enter an argument for the ngram size"<<endl;
		exit(-1);
	}
	if(arguments.size() > 1)
	{
		cerr<<"view_wordlength_stats: Too many arguments."<<endl;
		exit(-1);
	}

	size_t ngram_size = atoi(arguments[0].c_str());
	if(!ngram_size)
	{
		cerr<<"view_wordlength_stats: Either invalid argument, or there doesn't exist a "<<ngram_size<<"_grams.metadata file"<<endl;
		exit(-1);
	}

	auto relevant_metadata_itr = metadatas.find(ngram_size);
	if(relevant_metadata_itr == metadatas.end())
	{
		cerr<<"Unable to find metadata for n-gram collection of size " <<ngram_size;
		exit(-1);
	}
	Metadata &relevant_metadata = relevant_metadata_itr->second;
	if(!relevant_metadata.wordlength_stats_exist)
	{
		cerr<<"view_wordlength_stats: No wordlength statistics exist for "<<ngram_size<<"-grams, to generate them, use make_wordlength_stats"<<endl;
		exit(-1);
	}

	string filename = relevant_metadata.output_folder_name +string("/") + arguments[0] + string("_grams_wordlength_stats");

	FILE* outfile = fopen(filename.c_str(),"r");
	char buffer[16 + 1 + 16 + 1 + 1];
	cerr<<"Number of characters"<<"\t"<<"Frequency"<<endl;
	while(fgets(buffer, sizeof(buffer) /sizeof(buffer[0]), outfile))
	{
		char* endptr;	
		long long length = strtoll(buffer,&endptr,16);
		long long frequency = strtoll(endptr, NULL, 16);
		cout<<length<<"\t"<<frequency<<"\n";
	}	
	return;
}

