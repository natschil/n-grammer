//The following is slightly unprofessional, and should at some point be changed.
#include <iostream>
#include "ngram.counting.h"

using namespace std;

void print_usage(char* argv[])
{
    cerr << "Usage: " << argv[0] << " N input [outputdir = ./processing] [options]\n";
    cerr<<"\tWhere N is the size of the ngrams you want to count.\n";
    cerr<<"\tAnd options is one or many of:\n";
    cerr<<"\t\t--wordsearch-index-depth=k	(Split the word into k groups and build an index for each combination of these groups)\n";
    cerr<<endl;
}

int main (int argc, char* argv[])
{

  if((argc < 3) || (argc > 6))
  {
	  print_usage(argv);
          exit(1);
  }

  unsigned int ngramsize = atoi(argv[1]);
  if(!ngramsize)
  {
    cerr << "The first parameter is not a valid number"<<endl;
    print_usage(argv);
    exit(1);
  }

  FILE* f = NULL;
  const char* filename = argv[2];
  if(!strncmp(argv[2],"--",2))
  {
	  cerr<<"Please give an input file name"<<endl;
	  print_usage(argv);
	  exit(1);
  }
  f = fopen(filename,"r");
  if(!f)
  {
	  cerr<<"Could not open "<<filename<<" for reading"<<endl;
	  exit(1);
  }

  const char* outdir =NULL;
  int options_start = 2;

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
			cerr<<"\nInvalid option"<<argv[i]<<"\n\n";
			print_usage(argv);
			exit(1);
		}else if(!strncmp(argv[i],"--wordsearch-index-depth=",strlen("--wordsearch-index-depth=")))
		{
			char* ptr = argv[i] + strlen("--wordsearch-index-depth=");
			wordsearch_index_depth = atoi(ptr);
			if(!(wordsearch_index_depth > 0) || (wordsearch_index_depth > ngramsize) || (wordsearch_index_depth > MAX_INDICES))
			{
				cerr<<"\nInvalid parameter for --wordsearch-index-depth\n\n";
				print_usage(argv);
				exit(1);
			}
		}else
		{
			cerr<<"\nInvalid option"<<argv[i]<<"\n\n";
			print_usage(argv);
			exit(1);
		}
	}
  }

  
  count_ngrams(ngramsize,f,outdir,(unsigned int) wordsearch_index_depth);
  return 0; 
}
