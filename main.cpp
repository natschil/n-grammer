//The following is slightly unprofessional, and should at some point be changed.
#include <iostream>
#include <omp.h>
#include "ngram.counting.h"

using namespace std;

void print_usage(char* argv[])
{
    cerr << "Usage: " << argv[0] << " N input [outputdir = ./processing] [options]\n";
    cerr<<"\tWhere N is the size of the ngrams you want to count.\n";
    cerr<<"\tAnd options is one or many of:\n";
    cerr<<"\t\t--wordsearch-index-depth=k\t(Split the word into k groups and build an index for each combination of these groups)\n";
    cerr<<"\t\t--cache-entire-file\t\t(Whether or not to tell the kernel to load the whole file into memory)\n";
    cerr<<"\t\t--numbuffers=b\t\t\t(Use b buffers internally, and with that at maximum b threads)\n";
    cerr<<endl;
}

int main (int argc, char* argv[])
{

  if((argc < 3) || (argc > 7))
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
  fclose(f);

  const char* outdir =NULL;
  int options_start = 3;

  if(argc >= 4 && strncmp(argv[3],"--",2))
  {
	options_start++;
	outdir = argv[3];
  }else
	  outdir = "./processing";

 //Some default options:
 unsigned int wordsearch_index_depth = 1;
 unsigned int numbuffers;
#ifdef _OPENMP
 numbuffers = omp_get_num_procs();
#else
 numbuffers = 1;
#endif

  bool cache_entire_file = false;

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
		}else if(!strncmp(argv[i],"--numbuffers=",strlen("--numbuffers=")))
		{
			char* ptr = argv[i] + strlen("--numbuffers=");
			numbuffers = atoi(ptr);
			if(!numbuffers)
			{
				cerr<<"\nInvalid parameter for --numbuffers\n\n";
				print_usage(argv);
				exit(1);
			}

		}else if(!strncmp(argv[i],"--cache-entire-file",strlen("--cache-entire-file")))
		{
			cache_entire_file = true;
		}else {
			cerr<<"\nInvalid option"<<argv[i]<<"\n\n";
			print_usage(argv);
			exit(1);
		}
	}
  }

  
  count_ngrams(ngramsize,filename,outdir,(unsigned int) wordsearch_index_depth,numbuffers,cache_entire_file);
  return 0; 
}
