#include "util.h"


vector<string> split_ignoring_empty(const string& input_string,const char delim)
{
	istringstream ss(input_string);
	string cur;
	vector<string> result;
	while(getline(ss,cur,delim))
	{
		if(!cur.empty())
			result.push_back(cur);
	}
	if(!cur.empty())
		result.push_back(cur);
	return result;
}

vector<string> split(const string &input_string, const char delim)
{
	istringstream ss(input_string);
	string cur;
	vector<string> result;
	while(getline(ss,cur,delim))
	{
		result.push_back(cur);
	}
	if(!cur.empty())
		result.push_back(cur);
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
