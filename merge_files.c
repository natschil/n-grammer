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
	if(argc <=3)
	{
		fprintf(stderr, "Usage: %sinput_file_1 input_file_2\n");
		exit(-1);
	}
	char foldername[256]; //Should be large enough
	if(snprintf(foldername,256, "tmp_merge_%d", (int) getpid()) >= 256)
	{
		fprintf(stderr, "This should never happen and is a bug, exiting...\n");
		exit(-1);
	}
	char command[256];
	if(snprintf(command, 256, "rm -r %s > /dev/null 2>/dev/null",foldername) >= 256)
	{
		fprintf(stderr, "This is a bug\n");
		exit(-1);
	}
	system(command);
	if(snprintf(command,256, "mkdr -p %s",foldername) >= 256)
	{
		fprintf(stderr, "This is a bug\n");
		exit(-1);
	}
	if(system(command))
	{
		fprintf(stderr, "Failed to create folder %s\n", foldername);
		exit(-1);
	}
	FILE* first = fopen(argv[1],"r");
	if(!first)
	{
		fprintf(stderr,"Unable to open file %s", argv[1]);
		exit(-1);
	}
	FILE* second = fopen(argv[2], "r");
	if(second)
	{
		fprintf(stderr,"Unable to open file %s", argv[2]);
		exit(-1);
	}
	merge_files(first,second,stdout,512);




	return 0;
}
