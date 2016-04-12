#include "util.h"


vector<string> split_ignoring_empty(const string& input_string,const char delim)
{
	vector<string> result;
	auto itr = input_string.begin();
	auto eow = itr;
	for(;;itr++)
	{
		if((itr == input_string.end()) || (*itr == delim))
		{
			if(itr - eow)
			{
				result.push_back(string(eow,itr));
			}

			eow = (itr == input_string.end())? itr : itr+1;
		}
		if(itr == input_string.end())
			break;
	}	
	return result;
}

vector<string> split(const string &input_string, const char delim)
{
	vector<string> result;
	auto itr = input_string.begin();
	auto eow = itr;
	for(;;itr++)
	{
		if((itr == input_string.end()) || (*itr == delim))
		{
			result.push_back(string(eow,itr));
			eow = (itr == input_string.end())? itr : itr+1;
		}
		if(itr == input_string.end())
			break;
	}	
	return result;
}

string join(const vector<string> &to_join,const string &delim)
{
	ostringstream result{""};
	for(size_t i = 0; i < to_join.size(); i++)
	{
		result<<to_join[i];
		if((i+1) != to_join.size())
		{
			result<<delim;
		}
	}
	return result.str();
}

function<unsigned int(string)> string_to_unsigned_int = [](string in)
{
	return stoi(in);
};

function<string(unsigned int)> unsigned_int_to_string = [](unsigned int in)
{
	return to_string(in);
};
