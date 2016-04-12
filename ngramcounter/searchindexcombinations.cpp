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

unsigned int* CombinationIterator::operator*()
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
	unsigned int* value = (unsigned int*) malloc(totalsize * sizeof(*value));

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

optimized_combination::optimized_combination(unsigned int* unoptimized_combination, unsigned int ngramsize)
{
	int* tmp = (int*) malloc(sizeof(*tmp) * ngramsize);
	for(unsigned int i = 0; i< ngramsize; i++)
	{
		tmp[i] = unoptimized_combination[i] - unoptimized_combination[0];
	}
	lower_size = unoptimized_combination[0];
	upper_size = ngramsize - unoptimized_combination[0];

	lower = (unsigned int*) malloc(sizeof(*lower) * lower_size +1);//Because malloc(0) could theoretically return NULL
	upper =(unsigned int*) malloc(sizeof(*upper) * upper_size);

	for(unsigned int i = 0; i < ngramsize; i++)
	{
		for(unsigned int j = 0; j < ngramsize; j++)
		{
			if(unoptimized_combination[j] == i)
				tmp[i] = j;
		}
	}

	for(unsigned int i= 0; i < lower_size; i++)
	{
		lower[i] = tmp[lower_size -1 -i];
	}

	for(unsigned int j = 0; j < upper_size;j++)
	{
		upper[j] = tmp[ngramsize - upper_size + j];
	}
	free(tmp);
	return;
}
optimized_combination::optimized_combination(const optimized_combination &old)
{
	this->lower_size = old.lower_size;
	this->upper_size = old.upper_size;
	this->lower = (unsigned int*) malloc(sizeof(*this->lower) * lower_size + 1);
	memcpy(this->lower,old.lower,lower_size*sizeof(*this->lower));
	this->upper = (unsigned int*) malloc(sizeof(*this->upper) * upper_size);
	memcpy(this->upper,old.upper,upper_size*sizeof(*this->upper));
	return;
}
optimized_combination::~optimized_combination()
{
	free(lower);
	free(upper);
}

optimized_combination optimized_combination::operator=(const optimized_combination &old)
{
	this->lower_size = old.lower_size;
	this->upper_size = old.upper_size;
	this->lower = (unsigned int*) malloc(sizeof(*this->lower) * lower_size + 1);
	memcpy(this->lower,old.lower,lower_size*sizeof(*this->lower));
	this->upper = (unsigned int*) malloc(sizeof(*this->upper) * upper_size);
	memcpy(this->upper,old.upper,upper_size*sizeof(*this->upper));
	return *this;
};

