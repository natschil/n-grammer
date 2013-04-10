//The following is slightly unprofessional, and should at some point be changed.
#include <iostream>
#include "ngram.counting.h"

using namespace std;

int main (int argc, char* argv[])
{
  if((argc == 1) || (argc > 3))
  {
    cerr << "Usage: " << argv[0] << " N < input > output\n Where N is the size of the ngrams you want to count." << endl;
    cerr << "OR" <<argv[0] << " N input > output"<<endl;
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
  if(argc == 3)
  {
	f = fopen(argv[2],"r");
	if(!f)
	{
		cerr<<"Could not open file " <<argv[2]<<endl;
		exit(1);
	}

  }else
	f = stdin;

  
  double totalwords = analyze_ngrams(ngramsize,f);
  for(int j = 0; j < 256; j++)
  {
	  lexicon[j].clear();
  }
  fprintf(stderr,"Finished writing %f words\n", (double) totalwords);
  return 0; 
}
