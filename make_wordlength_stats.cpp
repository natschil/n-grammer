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
	auto relevant_metadata_itr = metadatas.find(ngram_size);
	if(!ngram_size || (relevant_metadata_itr == metadatas.end()))
	{
		cerr<<"make_wordlength_stats: Either invalid argument, or there doesn't exist a "<<ngram_size<<"_grams.metadata file"<<endl;
		exit(-1);
	}

	Metadata &relevant_metadata = relevant_metadata_itr->second;
	if(relevant_metadata.is_pos)
	{
		cerr<<"make_wordlength_stats: Cannot operate on POS indexes"<<endl;
		exit(-1);
	}
	if(relevant_metadata.indices.begin() == relevant_metadata.indices.end())
	{
		cerr<<"make_wordlength_stats: No indexes found for "<<ngram_size<<"-grams"<<endl;
		exit(-1);
	}

	auto first_index = *relevant_metadata.indices.begin();
	string index_specification = join(functional_map(first_index,unsigned_int_to_string),"_");

	string frequency_index_filename = relevant_metadata.output_folder_name + "/by_" + index_specification + "/0.out";
	string out_filename = relevant_metadata.output_folder_name +"/"	+ to_string(ngram_size) + "_grams_wordlength_stats";


	long long int *table = (long long int*) calloc(ngram_size * (relevant_metadata.max_word_size + 2),sizeof(*table));

	//We simply count the number of occurances of n-grams of each length.
	//A buffer for getline.
	char* line_buf = (char*) malloc(1024);
	size_t line_buf_size = 1024;

	//For every possible word length, the output file has the format <number of characters> <aggregate frequency>
	FILE* frequency_index_file = fopen(frequency_index_filename.c_str(),"r");
	if(!frequency_index_file)
	{
		free(line_buf); free(table);
		cerr<<"Unable to open "<<frequency_index_filename<<" for reading"<<endl;
		exit(-1);
	}
	ssize_t num_read;
	while((num_read = getline(&line_buf,&line_buf_size,frequency_index_file)) > 0)
	{
		const char* endptr = line_buf + num_read - 1; 
		endptr--; //Get rid of ending newline...
		while(isdigit(*endptr))
			endptr--;
		char* endptr2;
		long long number = strtoll(endptr,&endptr2,10);
		if((*endptr) != '\n'|| !number)
		{
			free(line_buf); free(table);
			fprintf(stderr,"make_wordlength_stats: Non-empty file to read has an invalid format\n");
			exit(-1);
		}
		table[endptr - line_buf] += number;
	}
	fclose(frequency_index_file);
	free(line_buf);

	FILE* outfile = fopen(out_filename.c_str(),"w");
	if(!outfile)
	{
		cerr<<"Could not open output file for reading"<<endl;
		exit(-1);
	}
	for(unsigned long long int i = 1; i < ngram_size * (relevant_metadata.max_word_size + 2); i++)
	{
		fprintf(outfile, "%16llX\t%16llx\n",i,table[i]); 
	}
	fclose(outfile);

	relevant_metadata.wordlength_stats_exist = 1;
	relevant_metadata.write();

	return;
}

