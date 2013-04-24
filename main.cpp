//The following is slightly unprofessional, and should at some point be changed.
#include <iostream>
#include "ngram.counting.h"

using namespace std;

void print_usage(char* argv[])
{
    cerr << "Usage: " << argv[0] << " N [input = stdin] [outputdir = ./processing] [options]\n";
    cerr<<"\tWhere N is the size of the ngrams you want to count.\n";
    cerr<<"\tAnd options is one or many of:\n";
    cerr<<"\t\t--wordsearch_index_until=k	(Split the word into k groups and build an index for each combination of these groups)\n";
}

int main (int argc, char* argv[])
{

  if((argc == 1) || (argc > 5))
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
  const char* outdir =NULL;
  if(argc >= 3)
  {
	f = fopen(argv[2],"r");
	if(!f)
	{
		cerr<<"Could not open file " <<argv[2]<<endl;
		exit(1);
	}

  }else
	f = stdin;

  if(argc >= 4)
  {
	outdir = argv[3];
  }else
	  outdir = "./processing";

 //Some default options:
  int wordsearch_index_depth = 1;


  if(argc > 4)
  {
	for(int i = 4; i<argc;i++)
	{
		if(strncmp(argv[i],"--",2))
		{
			print_usage(argv);
			exit(1);
		}else if(!strncmp(argv[i],"--wordsearch-index-depth",strlen("--wordsearch-index-depth")))
		{
			char* ptr = argv[i] + strlen("--wordsearch-index-depth");
			wordsearch_index_depth = atoi(ptr);
			if(!(wordsearch_index_depth > 0))
			{
				print_usage(argv);
				exit(1);
			}
		}
	}
  }

  
  count_ngrams(ngramsize,f,outdir,(unsigned int) wordsearch_index_depth);
  return 0; 
}
