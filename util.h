//(c) 2013 Nathanael Schilling
//This file contains various utility functions

#include <vector>
#include <string>
#include <sstream>
#include <functional>

#ifndef NGRAM_ANALYSIS_UTILS
#define NGRAM_ANALYSIS_UTILS
using namespace std;
vector<string> split_ignoring_empty(const string& inputstring,const char delim);
vector<string> split(const string& inputstring,const char delim);
string join(const vector<string> &to_join,const string &delim);

//Equivalent to a function that inductively defined, is so that:
//		find_nth(a,end, value,(n+1)) = find(find_nth(a,end,value,n),end,value);
//     and
//     		find_nth(begin,end,value,1) = find(begin,end,value)

template<class InputIt,class T> 
InputIt find_nth(InputIt begin, InputIt end, const T& value,size_t n)
{
	InputIt result = begin;
	while(n-- != 0)
	{
		find(result,end,value);
	}
	return result;
}

//Takes as parameters a vector<T> and mapfunction where mapfunction[T] gives U.
//Returns a vector<U> so that for each element i output[i] =  mapfunction(input[i])
template<class T,class U>
vector<U> functional_map(vector<T> input, function<U(T)> mapper)
{
	vector<U> result;
	for(auto &i : input)
	{
		result.push_back(mapper(i));
	}
	return result;
}
extern function<unsigned int(string)> string_to_unsigned_int;
extern function<string(unsigned int)> unsigned_int_to_string;


#endif
