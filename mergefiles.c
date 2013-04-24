//(c) 2013 Nathanael Schilling

#include "mergefiles.h"

static uint8_t scheduling_table[MAX_K][MAX_BUFFERS] = {{0}};
static int max_k = 0;


static void merge_next(int k, int n,int rightmost_run);
static int max_ngram_string_length = 40; //Given that this only serves as a hint, it doesn't matter whether or not it is set to the real value.

void init_merger(int new_max_ngram_string_length)
{
	max_ngram_string_length = new_max_ngram_string_length;
}




//Call this function with k,n referring to an n-gram collection that already exists.
//i.e. when the n-th nonterminal buffer has been written to disk, call schedule_next_merge(0,n,0)
//When the n-th buffer is also the last buffer, call schedule_next_merge(0,n,1)
void schedule_next_merge(int k, int n,int rightmost_run)
{
	if(n > MAX_BUFFERS)
	{
		fprintf(stderr, "Maximum number of buffers filled. Try adjusting MAX_BUFFERS in %s\n", __FILE__);
	}

	int other_n;

	if(n && rightmost_run) //Here we move the file up the merge tree until the next merge can be done "normally"
	{
		//We find the next highest "peak", and move the file there.
		int other_k = k;
		other_n = n - 1;
		while(other_n && (other_n % 2)) //Go the the next "peak" in the merge tree.
		{
			other_n = (other_n - 1)/2;
			other_k++;
		}

		if(other_n != (n - 1))
		{
			char old_directory_name[256];
			char new_directory_name[256];
			snprintf(old_directory_name, 256,"%d_%d",k,n);
			snprintf(new_directory_name, 256,"%d_%d", other_k,other_n + 1);
			remove(new_directory_name); //This shouldn't exist, so better be safe.
			rename(old_directory_name,new_directory_name);
			n = other_n + 1;
			k = other_k;
		}

	}else
	{
		other_n = (n && (n % 2)) ? n - 1: n + 1;
	}

	int run_next_merge;
	#pragma omp critical(merge_scheduler)
	{
		#pragma omp flush
		if(k > max_k)
			max_k = k;
		if(!(run_next_merge = scheduling_table[k][n]))
			scheduling_table[k][other_n] = rightmost_run ? 2 : 1;
		#pragma omp flush
	}
	if(run_next_merge)
	{
		#pragma omp task firstprivate(n,k,rightmost_run,run_next_merge)
		merge_next(k,n,rightmost_run || (run_next_merge == 2));
	}
}

//This function outputs the location of the output file.
//Only call this when you are sure that all merging has finished
//Returns -1 on error, though this is really an impossibility.
int get_final_k()
{
	#pragma omp flush
	return max_k;
}

static void merge_next(int k, int n,int rightmost_run)
{
	int other_n = (n && (n % 2)) ? n - 1: n+1;
	int final_n = ((n && (n % 2)) ? other_n : n) / 2;  //I.e. take the even one, divide by two
	int final_k = k + 1;
	char dirbuf[256];

	snprintf(dirbuf, 256,"./%d_%d",final_k,final_n);
	mkdir(dirbuf,S_IRUSR | S_IWUSR | S_IXUSR);
	fprintf(stderr, "Merging %d_%d with %d_%d to give %d_%d\n",k,n,k,other_n,final_k,final_n);

	#pragma omp flush //TODO: Research whether doing a flush here is neccessary, it probably isn't.
	int i;
	#pragma omp parallel for shared(other_n, final_n, final_k, n,k, max_ngram_string_length) private(i)
	for(i = 0; i< 256;i++)
	{

		char buf[256];
		char buf2[256];
		char output[256];
		snprintf(buf, 256, "./%d_%d/%d.out",k,n,i);
		snprintf(buf2, 256,"./%d_%d/%d.out",k,other_n,i);
		snprintf(output, 256,"./%d_%d/%d.out",final_k,final_n,i);

		FILE* firstfile = fopen(buf, "r");
		FILE* secondfile = fopen(buf2, "r");
		FILE* outputfile = fopen(output, "w");
		if(!firstfile)
		{
			fprintf(stderr, "Unable to open %s for reading\n", buf);
			exit(-1);
		}else if(!secondfile)
		{
			fprintf(stderr,"Unable to open %s for reading\n", buf2);
			exit(-1);
		}else if(!outputfile)
		{
			fprintf(stderr, "Unable to open %s for writing\n",output);
			exit(-1);
		}
		int mergefile_out = merge_files(firstfile,secondfile,outputfile,max_ngram_string_length);
		fclose(firstfile);
		fclose(secondfile);
		fclose(outputfile);
		if(mergefile_out)
		{
			fprintf(stderr, "Failed to merge files %s and %s\n",buf,buf2);
			exit(-1);
		}
		remove(buf);
		remove(buf2);
	}

	snprintf(dirbuf,256,"%d_%d",k,n);
	remove(dirbuf);
	snprintf(dirbuf,256,"%d_%d",k,other_n);
	remove(dirbuf);
	schedule_next_merge(final_k,final_n,rightmost_run);
	return;
}


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
	int get_first;

	char* endptr;

getbothfiles:
	get_first = 0;
	if((buf_read = getline(&buf, &buf_size, in_first)) < 3) 
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
		//We 
		{
			if((buf2_read = getline(&buf2, &buf2_size,in_second)) < 3)
			{
				*ptr1 = '\t';
				fputs(buf,out);
				copy_rest_of_file_to_output(in_first,out);
				free(buf);
				free(buf2);
				return 0;
			}else
			{
				second_number = 0;
				ptr2 = buf2 + buf2_read -2  ;
				while(isdigit(*ptr2)) ptr2--;
				*ptr2 = '\0';
				second_number = strtoll(ptr2+1,&endptr,10);
				if(*endptr != '\n')
				{
					fprintf(stderr,"Input file is wrongly formatted\n");
					free(buf);
					free(buf2);
					return -1;
				}
			}
	
		}else
		{
			if((buf_read = getline(&buf, &buf_size, in_first)) < 3) 
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
			first_number = 0;
			ptr1 = buf + buf_read -2  ;
			while(isdigit(*ptr1)) ptr1--;
			*ptr1 = '\0';
			first_number = strtoll(ptr1+1,&endptr,10);
			if(*endptr != '\n')
			{
				fprintf(stderr,"Input file is wrongly formatted\n");
				free(buf);
				free(buf2);
				return -1;
			}
		}

		int cmp = strcmp(buf, buf2);
		if(!cmp)
		{
			long long int finalnum = first_number + second_number;
			fputs(buf,out);
			fprintf(out,"\t%lld\n",finalnum);
			//Given that we've written the relevant output for both lines
			//We make sure that the previous buffer isn't written to disk if we reach
			//The end of a file now
			*buf2 = '\0';
			*buf = '\0';
			goto getbothfiles;

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
