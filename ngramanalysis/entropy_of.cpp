#include "entropy_of.h"

void entropy_of( map<unsigned int,Metadata> &metadatas,vector<string> arguments)
{
	vector<pair<vector<string>, long long int> > search_result =  internal_search(metadatas,arguments);

	long long number_of_outputs = accumulate(search_result.begin(),search_result.end(),0ll,
			[](long long current_value,const pair<vector<string>,long long int> &first)
			{
				return  current_value + first.second;
			}
			);
	if(!number_of_outputs)
	{
		cerr<<"Did not find anything using search string '"<<arguments[0]<<"'"<<endl;
		exit(-1);
	}
	long double entropy = accumulate(search_result.begin(),search_result.end(),(long double)0,
			[&number_of_outputs](long double current_value, const pair<vector<string>,long long int> &cur)
			{
				long double p = cur.second/(long double)number_of_outputs;
				return current_value + (p * log2l(p));

			}
			);
	cerr<<"Entropy (in bits)\n";
	entropy *= -1;
	if(entropy == 0)
		entropy = 0;
	cout<<entropy<<endl;
	return;
}
