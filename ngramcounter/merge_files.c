//(c) 2013 Nathanael Schilling
//This program will merge 2 files containing lines of the form
//<utf8 string that does not contain digits or punctuation><\t><integer>
//Writing the output to stdout that then is sorted by the strings with the integers  added.
//This program exists mainly for testing.

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include "mergefiles.h"

int main(int argc, char* argv[])
{
	if(argc < 3)
	{
		fprintf(stderr, "Usage: %s input_file_1 input_file_2\n",argv[0]);
		exit(-1);
	}
	FILE* first = fopen(argv[1],"r");
	if(!first)
	{
		fprintf(stderr,"Unable to open file %s", argv[1]);
		exit(-1);
	}
	FILE* second = fopen(argv[2], "r");
	if(!second)
	{
		fprintf(stderr,"Unable to open file %s", argv[2]);
		exit(-1);
	}
	struct stat first_stat;
	struct stat second_stat;
	stat(argv[1],&first_stat);
	stat(argv[2], &second_stat);

	FILE* outfile = stdout;
	if(argc == 4)
	{
		outfile = fopen(argv[3],"w+");
	}

	merge_files(first,first_stat.st_size,second,second_stat.st_size,outfile,512);
	fclose(first);
	fclose(second);
	return 0;
}
