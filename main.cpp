/* (c) 2013 Nathanael Schilling
 * This program is intended for use with ngram.counter for analyzing the data created by it
 */

#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>

#include <vector>
#include <map>

#include "manage_metadata.h"
#include "invert_index.h"
#include "search.h"

void print_usage(int argc,char* argv[]);


int main(int argc, char* argv[])
{
	if(argc <= 2)
	{
		print_usage(argc,argv);
		return -1;
	}

	DIR* processing_folder = opendir(argv[1]);
	if(!processing_folder)
	{
		cerr<<"Cannot open directory "<<argv[1]<<endl;
		return -1;
	}

	map<unsigned int, Metadata> metadatas;

	struct dirent* current_entry;
	while((current_entry = readdir(processing_folder)))
	{
		if(isdigit(*(current_entry->d_name)))
		{
			char* ptr = current_entry->d_name+1;
			while(isdigit(*ptr))
				ptr++;

			if(strncmp(ptr,"_grams.metadata",strlen("_grams.metadata")))
				continue;

			try
			{
				string filename(current_entry->d_name);
				string foldername(argv[1]);
				metadatas[atoi(current_entry->d_name)] = Metadata(filename,foldername);
			}catch(int i)
			{
			}
		}
	}
	closedir(processing_folder);

	string prev_filename;
	int first = 1;
	if(!metadatas.size())
	{
		fprintf(stderr,"No valid metadata found in directory %s\n",argv[1]);
		exit(-1);
	}
	for(map<unsigned int,Metadata>::iterator i = metadatas.begin(); i != metadatas.end();i++)
	{
		if(!first)
		{
			if((*i).second.file_name != prev_filename)
			{
				fprintf(stderr,"The processing directory contains data from different files %s and %s, exiting\n",
						prev_filename.c_str(), (*i).second.file_name.c_str());
				exit(-1);
			}
		}
		prev_filename = (*i).second.file_name;
	}

	string command(argv[2]);
	vector<string> arguments;
	for(int i = 3; i< argc;i++)
	{
		arguments.push_back(argv[i]);
	}
	if(command == "invert_index")
	{
		invert_index(metadatas,arguments);
	}else if(command == "search")
	{
		do_search(metadatas,arguments);
	}
	return 0;

}


void print_usage(int argc,char* argv[])
{
	fprintf(stderr, "Usage %s <ngram.counting output folder> COMMAND OPTIONS\n",argv[0]);
	fprintf(stderr, "COMMAND is one of:\n");
	fprintf(stderr,"\t\tinvert_index\n");
	fprintf(stderr,"\t\tsearch\n");
	fflush(stderr);
	return;
}
