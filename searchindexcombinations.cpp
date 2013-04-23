#include "searchindexcombinations.h"

CombinationIterator::CombinationIterator(int n)
{
	this->currentbitstring = vector<int>(n);
	for(int i=0; i< n; i++)
	{
		currentbitstring[i] = 1;
	}
	this->n = n;
	return;
}

CombinationIterator::~CombinationIterator()
{
	return;
}

int* CombinationIterator::operator*()
{
	vector<int> A;
	vector<int> B;
	vector<int> C;
	for(int i = 0; i < n; i++)
	{
		if(currentbitstring[i])
		{
			B.push_back(i);
		}else
		{
			A.push_back(i);
			if(B.size())
			{
				C.push_back(B.back());
				B.pop_back();
			}
		}
	}
	sort(A.begin(), A.end());
	sort(B.begin(), B.end());
	sort(C.begin(),C.end());
	int totalsize = A.size() + B.size() + C.size();
	int* value = (int*) malloc(totalsize * sizeof(*value));

	int sofar = 0;
	for(size_t i = 0; i< A.size();i++)
	{
		value[sofar] = A[i];
		sofar++;
	}
	for(size_t i= 0; i< B.size(); i++)
	{
		value[sofar] = B[i];
		sofar++;
	}
	for(size_t i = 0; i< C.size(); i++)
	{
		value[sofar] = C[i];
		sofar++;
	}

	return value;
}

bool CombinationIterator::operator++()
{

	int height = 0;
	for(int i = 0; i< n; i++)
	{
		if(currentbitstring[i])
			height++;
		else
			height--;
	}

	for(int i = n - 1; i>= 0;i--)
	{
		if(!currentbitstring[i])
		{
			height++;
		}else if(height > 1)
		{
			currentbitstring[i] = 0;
			for(int j = i + 1; j<n;j++)
			{
				currentbitstring[j] = 1;
			}

			break;
		}else if(i == 0)
		{
			return false;
		}else
		{
			height--;
		}

	}
	return true;
}
