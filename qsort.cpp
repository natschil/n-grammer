#include "qsort.h"

void my_qsort(word* base,size_t nmeb)
{
	int M = 1; //TODO: make M configurable

	//See "The Art of Computer Programming" (Referenced above) for context for the comments of this algorithm.
	//The comments assume that the reader is familiar with the said algorithm.

	//We use sequential allocation for the stack.
	//At some point we could consider whether or not this is a good idea.
	int* stack = (int*) malloc( 2 * sizeof(int) * (floor(log2(nmeb)) + 1) ); //The stack needs only lg(N) amounts of pairs. However, to simplify the code, we use one pair more on the stack so that we can loop over the pairs more easier.
	int* stack_ptr = stack;

	*(stack_ptr++) = 0; //We push the initial values onto the stack:
	*(stack_ptr++) = nmeb - 1;

	int i,j,r,l;
	while(1)
	{
		//If there are no items to pop off the stack, the data is sorted:
		if(stack_ptr == stack)
			break;

		//We pop the stack
		r = *(--stack_ptr);
		l = *(--stack_ptr);


		for(i = l ,j = r + 1; (i + 1)<j;)
		{

			if( strcmp((const char*)(base + l)->contents,(const char*)(base + (i+1))->contents) > 0)
			{
				i++;
				continue;
			}else
			{
				j--;
				for(;(i +1)<j;j--)
				{
					if( strcmp((const char*) (base + l)->contents,(const char*)(base + j)->contents) > 0)
					{
						//Here we exchange the two items.
						std::swap(*(base + j), *(base + (i +1)));
						i++;
						break;
					}
				}
			}
		} 

		if(i!=l)
		{
			std::swap(*(base + l), *(base + i));
		}

		//In this case, the file should now be quite nicely partitioned.
		//i + 1 should point to the first element of the subfile to the right.
		//At this point, we push onto the stack the two subfiles, pushing the smaller one on first
		//As that one is immediately popped off on the next iteration anyways.
		//Of course, there is still the possibilty that the sort has finished. In this case, i,j,r are equal
		int ordering;
		switch(ordering = ((i -l) > (r - i )))
		{
			case 1:
			if( r - (i + 1) > (M + 1))
			{
				*(stack_ptr++) = i + 1;
				*(stack_ptr++) = r;
			}
			case 0:
			if((i - 1) - l > (M + 1))
			{
				*(stack_ptr++) = l;
				*(stack_ptr++) = (i-1);
			}
			if(ordering) break;
			default:
			if( r - (i + 1) > (M + 1))
			{
				*(stack_ptr++) = i+1;
				*(stack_ptr++) = r;
			}
		}

	}
	
	return;
}


