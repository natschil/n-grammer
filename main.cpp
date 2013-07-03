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
    cerr<<"\t\t--build-wordsearch-indexes\t(Build indexes for multiattribute retrieval)\n";
    cerr<<"\t\t--build-wordsearch-index=n\t(Build the n-th index for multiatttribute retrieval\n";
    cerr<<"\t\t--build-wordsearch-indexes-howmany=d\t(Build d indexes at a time, starting from the n-th index (see above))\n";
    cerr<<"\t\t--cache-entire-file\t\t(Whether or not to tell the kernel to load the whole file into memory)\n";
    cerr<<"\t\t--numbuffers=b\t\t\t(Use b buffers internally, and with that at maximum b threads)\n";
    cerr<<"\t\t--corpus-has-pos-data\t\t(The corpus contains 'part of speech' information)\n";
    cerr<<"\t\t--build-pos-supplement-indexes\t\t(Build only 'part of speech' supplement indexes)\n";
  //  cerr<<"\t\t--also-build-smaller-indexes\t\t(Also build (n-1)-grams and (n-2)-grams etc..\n";
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
 unsigned int numbuffers;
#ifdef _OPENMP
 numbuffers = omp_get_num_procs();
#else
 numbuffers = 1;
#endif

  bool cache_entire_file = false;
  bool has_pos = false;
  bool build_pos_supplement_indexes = false;

  bool build_all_wordsearch_indexes = false;
  int single_wordsearch_index_to_build = -1;
  int wordsearch_indexes_howmany = -1;


  if(argc > options_start)
  {
	for(int i = options_start; i<argc;i++)
	{
		if(strncmp(argv[i],"--",2))
		{
			cerr<<"\nInvalid option"<<argv[i]<<"\n\n";
			print_usage(argv);
			exit(1);
		}else if(!strcmp(argv[i],"--build-wordsearch-indexes"))
		{
			build_all_wordsearch_indexes = true;

		}else if(!strncmp(argv[i],"--build-wordsearch-index=",strlen("--build-wordsearch-index=")))
		{
			char* ptr = argv[i] + strlen("--build-wordsearch-index=");
			char* endptr;
			long int index_number = strtol(ptr,&endptr,10);
			if(*ptr && !*endptr && (index_number >= 0))
			{
				single_wordsearch_index_to_build = index_number;
			}else
			{
				cerr<<"Invalid or nonexistent input for --build-wordsearch-index"<<endl;
				print_usage(argv);
				exit(-1);
			}
		}else if(!strncmp(argv[i],"--build-wordsearch-indexes-howmany=",strlen("--build-wordsearch-indexes-howmany=")))
		{
			char* ptr = argv[i] + strlen("--build-wordsearch-indexes-howmany=");
			char* endptr;
			wordsearch_indexes_howmany = strtol(ptr, &endptr,10);
			if(!(*ptr && !*endptr && wordsearch_indexes_howmany > 0))
			{
				cerr<<"Invalid or nonexistent parameter for --build-wordsearch-indexes-howmany"<<endl;
				print_usage(argv);
				exit(-1);
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

		}else if(!strcmp(argv[i],"--cache-entire-file"))
		{
			cache_entire_file = true;
		}else  if(!strcmp(argv[i],"--corpus-has-pos-data"))
		{
			has_pos = true;
		}else if(!strcmp(argv[i],"--build-pos-supplement-indexes"))
		{
			build_pos_supplement_indexes = true;
		}else
		{
			cerr<<"\nInvalid option"<<argv[i]<<"\n\n";
			print_usage(argv);
			exit(1);
		}
	}
  }

  //Some small checks to catch out simple errors.
  if(build_pos_supplement_indexes)
	  has_pos = true;

  if(build_all_wordsearch_indexes)
  {
  	if(wordsearch_indexes_howmany > 0)
  	{
		cerr<<"Please set either --build-wordsearch-indexes or --build-wordsearch-indexes-howmany but not both"<<endl;
		print_usage(argv);
		exit(-1);
	}
	  
	if(single_wordsearch_index_to_build>0)
	{
		cerr<<"Please set either --build-wordsearch-indexes or --build-wordsearch-index=n but not both"<<endl;
		print_usage(argv);
		exit(-1);
	}
  }else
  {
  	//Insert defaut values where no values have been set.
	if((wordsearch_indexes_howmany < 0 ) && (single_wordsearch_index_to_build < 0))
	{
		single_wordsearch_index_to_build = 0;
		wordsearch_indexes_howmany = 1;
	}else if((wordsearch_indexes_howmany > 0) && (single_wordsearch_index_to_build < 0))
		single_wordsearch_index_to_build = 0;
	else if((single_wordsearch_index_to_build >= 0) && (wordsearch_indexes_howmany < 0))
		  wordsearch_indexes_howmany = 1;
  }
	  
  count_ngrams(
		  ngramsize,
		  filename,
		  outdir,
		  build_all_wordsearch_indexes,
		  numbuffers,
		  cache_entire_file,
		  has_pos,
		  build_pos_supplement_indexes,
		  single_wordsearch_index_to_build,
		  wordsearch_indexes_howmany
		  );
  return 0; 
}
