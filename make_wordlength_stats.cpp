#include "make_wordlength_stats.h"

void make_wordlength_stats(map<unsigned int,Metadata> &metadatas,vector<string> &arguments)
{
	if(!arguments.size())
	{
		cerr<<"make_wordlenth_stats: Missing argument, please enter an argument for the ngram size"<<endl;
		exit(-1);
	}
	if(arguments.size() > 1)
	{
		cerr<<"make_wordlength_stats: Too many arguments."<<endl;
		exit(-1);
	}

	size_t ngram_size = atoi(arguments[0].c_str());
	if(!ngram_size || (metadatas.find(ngram_size) == metadatas.end()))
	{
		cerr<<"make_wordlength_stats: Either invalid argument, or there doesn't exist a "<<ngram_size<<"_grams.metadata file"<<endl;
		exit(-1);
	}

	Metadata &relevant_metadata = metadatas[ngram_size];
	if(relevant_metadata.indices.begin() == relevant_metadata.indices.end())
	{
		cerr<<"No indexes found for "<<ngram_size<<"-grams"<<endl;
		exit(-1);
	}
	stringstream index_as_string_stream;
	for(unsigned int i = 0; i< ngram_size; i++)
	{
		index_as_string_stream << "_";
		index_as_string_stream << (*relevant_metadata.indices.begin())[i];
	}
	string filename = relevant_metadata.folder_name + string("/") + string("by") + index_as_string_stream.str() + string("/0.out"); 
	string out_filename = relevant_metadata.folder_name +string("/"	) + arguments[0] + string("_grams_wordlength_stats");


	long long int *table = (long long int*) calloc(ngram_size * (MAX_WORD_SIZE + 2),sizeof(*table));

	//The first pass of a distribution sort.
	const char* filename_c_str = filename.c_str();

	//A buffer for getline.
	char* string_buf = (char*) malloc(1024);
	size_t string_buf_size = 1024;

	//For every possible word length, the output file has the format <number of characters> <aggregate frequency>
	FILE* currentfile = fopen(filename_c_str,"r");
	if(!currentfile)
	{
		free(string_buf); free(table);
		cerr<<"Unable to open "<<filename<<" for reading"<<endl;
		exit(-1);
	}
	ssize_t num_read;
	while((num_read = getline(&string_buf,&string_buf_size,currentfile)) > 0)
	{
		const char* endptr = string_buf + num_read - 1; 
		endptr--; //Get rid of ending newline...
		while(isdigit(*endptr))
			endptr--;
		char* errptr;
		long long number = strtoll(endptr,&errptr,10);
		if((*errptr) != '\n'|| !number)
		{
			free(string_buf); free(table);
			fprintf(stderr,"make_wordlength_stats: Non-empty file to read has an invalid format\n");
			exit(-1);
		}
		table[endptr - string_buf] += number;
	}
	fclose(currentfile);
	free(string_buf);

	FILE* outfile = fopen(out_filename.c_str(),"w");
	if(!outfile)
	{
		cerr<<"Could not open output file for reading"<<endl;
		exit(-1);
	}
	for(unsigned long long int i = 1; i < ngram_size * (MAX_WORD_SIZE + 2); i++)
	{
		fprintf(outfile, "%16llX\t%16llx\n",i,table[i]); 
	}
	fclose(outfile);

	relevant_metadata.wordlength_stats_exist = 1;
	relevant_metadata.write();

	return;
}

