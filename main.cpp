/* (c) 2013 Nathanael Schilling
 * This program is intended for use with ngram.counter for analyzing the data created by it
 */

#include<stdio.h>

void print_usage(int argc,char* argv[]);


int main(int argc, char* argv[])
{
	if(argc == 1)
	{
		print_usage(argc,argv);
		return 0;
	}
}


void print_usage(int argc,char* argv[])
{
	fprintf(stderr, "Usage %s COMMAND OPTIONS\n",argv[0]);
	fprintf(stderr, "COMMAND is one of:\n");
	fprintf(stderr,"\t\tinvert_index\n");
	fflush(stderr);
	return;
}
