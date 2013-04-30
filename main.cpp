//The following is slightly unprofessional, and should at some point be changed.
#include <iostream>
#include "ngram.counting.h"

using namespace std;

void print_usage(char* argv[])
{
    cerr << "Usage: " << argv[0] << " N [input = stdin] [outputdir = ./processing] [options]\n";
    cerr<<"\tWhere N is the size of the ngrams you want to count.\n";
    cerr<<"\tAnd options is one or many of:\n";
    cerr<<"\t\t--wordsearch-index-depth=k	(Split the word into k groups and build an index for each combination of these groups)\n";
    cerr<<endl;
}

int main (int argc, char* argv[])
{

  if((argc == 1) || (argc > 6))
  {
	  print_usage(argv);
          exit(1);
  }

  char* number = argv[1];
  char* errptr;
  unsigned int ngramsize = (unsigned int) strtol(number, &errptr,10);
  if(!ngramsize || (errptr && *errptr))
  {
    cerr << "The first parameter is not a valid number"<<endl;
    print_usage(argv);
    exit(1);
  }

  FILE* f = NULL;
  const char* filename = argv[2];

  const char* outdir =NULL;
  int options_start = 2;
  if(argc >= 3 && strncmp(argv[2],"--",2))
  {
	options_start++;
	f = fopen(argv[2],"r");
	if(!f)
	{
		cerr<<"Could not open file " <<argv[2]<<endl;
		exit(1);
	}

  }else
	filename = NULL;

  if(argc >= 4 && strncmp(argv[3],"--",2))
  {
	options_start++;
	outdir = argv[3];
  }else
	  outdir = "./processing";

 //Some default options:
  unsigned int wordsearch_index_depth = 1;


  if(argc > options_start)
  {
	for(int i = options_start; i<argc;i++)
	{
		if(strncmp(argv[i],"--",2))
		{
			fprintf(stderr,"\nInvalid option %s\n\n",argv[i]);
			print_usage(argv);
			exit(1);
		}else if(!strncmp(argv[i],"--wordsearch-index-depth=",strlen("--wordsearch-index-depth=")))
		{
			char* ptr = argv[i] + strlen("--wordsearch-index-depth=");
			wordsearch_index_depth = atoi(ptr);
			if(!(wordsearch_index_depth > 0) || (wordsearch_index_depth > ngramsize) || (wordsearch_index_depth > MAX_INDICES))
			{
				fprintf(stderr,"\nInvalid parameter for --wordsearch-index-depth\n\n");
				print_usage(argv);
				exit(1);
			}
		}else
		{
			fprintf(stderr,"\nInvalid option %s\n\n",argv[i]);
			print_usage(argv);
			exit(1);
		}
	}
  }

  
  count_ngrams(ngramsize,filename,outdir,(unsigned int) wordsearch_index_depth);
  return 0; 
}
