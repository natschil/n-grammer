/* (c) 2013 Nathanael Schilling
 * This file implements an algorithm for finding the names of all combined indexes
 * needed for doing multiattribute search. 
 * For details of its implementation, see 
 *
 * 	Knuth, Donald. "The Art of Computer Programming, Volume 3 , Sorting and Searching, Second Edition".
 * 		 Addison-Wesley Publishing Company 1998. p 567,579
 * Or 
 * 	Shneiderman, Ben. "Reduced Combined Indexes for Multiple Attribute Retrival". http://www.cs.indiana.edu/pub/techreports/TR43.pdf, Appendix A
 * 	(which has a copy of the algorithm used in the Appendix)
 *
 *
 * 	Basically this algorithm gives all combinations C of a attributes 1..n so that for every ordered subset s of n of size t,
 * 	there exists an element in C so that the first t attributes of the element are the first t attributes of of s.
 * 	For example, for the attributes 0123, this algorithm generates
 * 				0123	1230	1302	2031	2301	3012
 *
 * 				As a simple example, if we look for the 3 attributes 341, they are the first three attribues of 3142
 *
 * 	For 123, it would generate
 * 				123	231	321
 *
 *
 */

#include <vector>
#include <algorithm>

using namespace std;

class CombinationIterator
{
public:
	CombinationIterator(int n);
	~CombinationIterator();
	bool operator++();

	//Returns 0 if this was the last one.
	unsigned int* operator*();

private:
	vector<int> currentbitstring;
	int n;
};

