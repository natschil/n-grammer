//The following is slightly unprofessional, and should at some point be changed.
#include <iostream>
#include "ngram.counting.h"

using namespace std;

int main (int argc, char* argv[])
{
  if((argc == 1) || (argc > 4))
  {
    cerr << "Usage: " << argv[0] << " N [input = stdin] [output = stdout]\n \tWhere N is the size of the ngrams you want to count.\n";
    exit(1);
  }

  char* number = argv[1];
  char* errptr;
  unsigned int ngramsize = (unsigned int) strtol(number, &errptr,10);
  if(!ngramsize || (errptr && *errptr))
  {
    cerr << "The first parameter is not a valid number"<<endl;
    cerr << "Usage: " << argv[0] << " N < input > output\n Where N is the size of the ngrams you want to count." << endl;
    cerr << "OR" <<argv[0] << " N input > output"<<endl;
    exit(1);
  }

  FILE* f = NULL;
  FILE* outfile =NULL;
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
	outfile = fopen(argv[3],"w");
	if(!outfile)
	{
		cerr<<"Could not open file "<<argv[2]<<endl;
		exit(1);
	}
  }else
	  outfile = stdout;

  
  struct timeval start_time,end_time,difference;
  gettimeofday(&start_time,NULL);
  long long int totalwords = analyze_ngrams(ngramsize,f,outfile);
  gettimeofday(&end_time,NULL);
  fprintf(stderr,"Finished writing %lld words and took %d seconds\n", (long long int) totalwords,end_time.tv_sec - start_time.tv_sec);
  return 0; 
}
