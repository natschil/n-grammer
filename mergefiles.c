//(c) 2013 Nathanael Schilling

#include "mergefiles.h"

static void copy_rest_of_file_to_output(FILE* input, FILE* output)
{
	int c;
	while((c = fgetc(input)) != EOF)
	{
		fputc(c,output);
	}
	return;
}

//Returns 0 on sucess, non-zero on error.
int merge_files(FILE* in_first, FILE* in_second, FILE* out,int max_ngram_string_length)
{
	//		   string_length	tab   ( length of number as text) '\0'
	size_t buf_size = max_ngram_string_length + 1 + floor(log10(LLONG_MAX)) + 1 + 1;
	char* buf = malloc(buf_size);
	ssize_t buf_read;

	size_t buf2_size = buf_size;
	char* buf2 = malloc(buf_size);
	ssize_t buf2_read;


	
	long long int first_number = 0;
	long long int second_number = 0;
	char* ptr1 = NULL;
	char* ptr2 = NULL;
	int get_first = 0;

	char* endptr;

	if((buf_read = getline(&buf, &buf_size, in_first)) < 0) 
	{
		//We write the rest of the second file to output.
		copy_rest_of_file_to_output(in_second,out);
		free(buf);
		free(buf2);
		return 0;
	}else
	{
		ptr1 = buf + buf_read - 2; //one for the newline, one for the null.
		while(isdigit(*ptr1)) ptr1--;
		*ptr1 = '\0';
		first_number = strtoll(ptr1+1,&endptr,10);
		if(*endptr != '\n')
		{
			fprintf(stderr,"Input file has wrong formatting");
			free(buf);
			free(buf2);
			return -1;
		}
	}

	while(1)
	{
		if(!get_first)
		{
			if((buf2_read = getline(&buf2, &buf2_size,in_second)) < 0)
			{
				*ptr1 = '\t';
				fputs(buf2,out);
				copy_rest_of_file_to_output(in_first,out);
				free(buf);
				free(buf2);
				return 0;
			}else
			{
				second_number = 0;
				char* ptr2 = buf2 + buf2_read -2  ;
				while(isdigit(*ptr2)) ptr2--;
				*ptr2 = '\0';
				second_number = strtoll(ptr2+1,&endptr,10);
				if(*endptr != '\n')
				{
					fprintf(stderr,"Input file is wrongly formatted");
					free(buf);
					free(buf2);
					return -1;
				}
			}
	
		}else
		{
			if((buf_read = getline(&buf, &buf_size, in_first)) < 0) 
			{
				*ptr2 = '\t';
				fputs(buf2,out);
				copy_rest_of_file_to_output(in_second,out);
				free(buf);
				free(buf2);
				return 0;
			}
			first_number = 0;
			ptr1 = buf + buf_read - 2; //one for the newline, one for the null.
			while(isdigit(*ptr1)) ptr1--;
			*ptr1 = '\0';		
	
		}

		int cmp = strcmp(buf, buf2);
		if(!cmp)
		{
			long long int finalnum = first_number + second_number;
			fputs(buf,out);
			fprintf(out,"%lld\n",finalnum);

		}else if(cmp < 0) //We write the first number.
		{
			*ptr1 = '\t';
			fputs(buf,out);
			*ptr1 = '\0';
			get_first = 1;
		}else
		{
			*ptr2 = '\t';
			fputs(buf2,out);
			*ptr2 = '\0';
			get_first = 0;
		}
	}
}	
