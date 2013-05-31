//(c) 2013 Nathanael Schilling

#include "mergefiles.h"

/*Function declarations of functions found in this file*/

void init_merger(int new_max_ngram_string_length);
void schedule_next_merge(int k, int n,int rightmost_run,uint8_t (*scheduling_table)[2*MAX_BUFFERS - 1],const char* prefix);
int get_final_k();
static void merge_next(int k, int n,int rightmost_run, uint8_t (*scheduling_table)[2*MAX_BUFFERS - 1],const char* prefix);
static void copy_rest_of_file_to_output(FILE* input,off_t inputsize, FILE* output);
int merge_files(FILE* in_first,off_t first_size, FILE* in_second,off_t second_size, FILE* out,int max_ngram_string_length);


/*Global variables */

static int max_k = 0;
//Given that this only serves as a hint, it doesn't matter whether or not it is set to the real value.
static int max_ngram_string_length = 40; 


/*Function definitions*/

void init_merger(int new_max_ngram_string_length)
{
	max_ngram_string_length = new_max_ngram_string_length;
}

//Call this function with k,n referring to an n-gram collection that already exists.
//i.e. when the n-th nonterminal buffer has been written to disk, call schedule_next_merge(0,n,0)
//When the n-th buffer is also the last buffer, call schedule_next_merge(0,n,1)
void schedule_next_merge(int k, int n,int rightmost_run,uint8_t (*scheduling_table)[2*MAX_BUFFERS - 1],const char* prefix)
{
	if(n > MAX_BUFFERS)
	{
		fprintf(stderr, "Maximum number of buffers filled. Try adjusting MAX_BUFFERS in config.h\n");
		exit(-1);
	}

	int other_n;
	//Whether or not this is the final merge..
	if(n && rightmost_run) 
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
			//MARKER8
			char* old_directory_name = malloc(strlen(prefix) + 1 + 4 + 1 + 4 + 1);
			char* new_directory_name = malloc(strlen(prefix) + 1 + 4 + 1 + 4 + 1);

			snprintf(old_directory_name, strlen(prefix) + 1 + 4 + 1 + 4 + 1,"%s/%d_%d",prefix,k,n);
			snprintf(new_directory_name, strlen(prefix) + 1 + 4 + 1 + 4 + 1,"%s/%d_%d", prefix,other_k,other_n + 1);
			remove(new_directory_name); //This shouldn't exist, so better be safe.
			rename(old_directory_name,new_directory_name);
			n = other_n + 1;
			k = other_k;
			free(old_directory_name);
			free(new_directory_name);
		}
	}else
	{
		other_n = (n && (n % 2)) ? n - 1: n + 1;
	}

	int run_next_merge;
	//TODO: Do this with openmp locks instead.
	#pragma omp critical(merge_scheduler)
	{
		#pragma omp flush
		if(k > max_k)
			max_k = k;
		if(!(run_next_merge = (*scheduling_table)[2*MAX_BUFFERS-(MAX_BUFFERS/(1 << k))+n]))
			(*scheduling_table)[2*MAX_BUFFERS-(MAX_BUFFERS/(1<<k)) + other_n] = rightmost_run ? 2 : 1;
		#pragma omp flush
	}

	if(run_next_merge)
	{
		fprintf(stdout,"|");
		fflush(stdout);
		//#pragma omp task firstprivate(n,k,rightmost_run,run_next_merge,scheduling_table,prefix) default(none)
		merge_next(k,n,rightmost_run || (run_next_merge == 2),scheduling_table,prefix);
		fprintf(stdout,"|");
		fflush(stdout);
	}
}

//This function returns the value of the "deepest" value 
int get_final_k()
{
	#pragma omp flush
	return max_k;
}

static void merge_next(int k, int n,int rightmost_run, uint8_t (*scheduling_table)[2*MAX_BUFFERS-1],const char* prefix)
{
	int other_n = (n && (n % 2)) ? n - 1: n+1;
	int final_n = ((n && (n % 2)) ? other_n : n) / 2;  //I.e. take the even one, divide by two
	int final_k = k + 1;
	//MARKER8
	char* dirbuf = malloc(strlen(prefix) + 1 + 4 + 1 + 4 + 1);

	snprintf(dirbuf, strlen(prefix) + 1 + 4 + 1 + 4 + 1,"%s/%d_%d",prefix,final_k,final_n);
	mkdir(dirbuf,S_IRUSR | S_IWUSR | S_IXUSR);
	//fprintf(stdout, "Merging %d_%d with %d_%d to give %d_%d\n",k,n,k,other_n,final_k,final_n);

	#pragma omp flush //TODO: Research whether doing a flush here is neccessary, it probably isn't.
	int i;
	for(i = 0; i< 1;i++)
	{

		//MARKER8
		char* buf = malloc(strlen(prefix)+1 + 4 + 1 + 4 + 1 +4 + 4 + 1);
		char* buf2 = malloc(strlen(prefix)+1 + 4 + 1 + 4 + 1 +4 + 4 + 1);
		char* output = malloc(strlen(prefix)+1 + 4 + 1 + 4 + 1 +4 + 4 + 1);

		sprintf(buf,  "%s/%d_%d/%d.out",prefix,k,n,i);
		sprintf(buf2, "%s/%d_%d/%d.out",prefix,k,other_n,i);
		sprintf(output, "%s/%d_%d/%d.out",prefix,final_k,final_n,i);

		struct stat firstfile_st;
		struct stat secondfile_st;
		if(stat(buf, &firstfile_st))
		{
			fprintf(stderr,"Unable to stat %s\n",buf);
			exit(-1);
		}
		if(!firstfile_st.st_size)//Simply move the other file to the final location.
		{
			rename(buf2,output);
			free(buf);
			free(buf2);
			free(output);
			continue;
		}
		if(stat(buf2,&secondfile_st))
		{
			fprintf(stderr,"Unable to stat %s\n",buf);
			exit(-1);
		}
		if(!secondfile_st.st_size)
		{
			rename(buf,output);
			free(buf);
			free(buf2);
			free(output);
			continue;
		}


		FILE* firstfile = fopen(buf, "r");
		FILE* secondfile = fopen(buf2, "r");
		FILE* outputfile = fopen(output, "w+");
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

		struct stat first_stat;
		struct stat second_stat;
		if( stat(buf,&first_stat) || stat(buf2, &second_stat))
		{
			fprintf(stderr, "Unable to stat either %s or %s", buf, buf2);
			exit(-1);
		}

		int mergefile_out = merge_files(firstfile,first_stat.st_size,secondfile,second_stat.st_size,outputfile,max_ngram_string_length);
		fclose(firstfile);
		fclose(secondfile);
		fclose(outputfile);
		if(mergefile_out)
		{
			fprintf(stderr, "Failed to merge files %s and %s\n",buf,buf2);
			exit(-1);
		}
		free(buf);
		free(buf2);
		free(output);
	}

	sprintf(dirbuf,"%s/%d_%d",prefix,k,n);
	remove(dirbuf);
	sprintf(dirbuf,"%s/%d_%d",prefix,k,other_n);
	remove(dirbuf);
	free(dirbuf);

	schedule_next_merge(final_k,final_n,rightmost_run,scheduling_table,prefix);
	return;
}


static void copy_rest_of_file_to_output(FILE* input,off_t inputsize, FILE* output)
{
	if( output == stdout)
	{
		int c;
		while( (c = fgetc(input)) != EOF)
		{
			fputc(c,output);
		}
		return;
	}else
	{
		if(inputsize == 0)
		       return;	
	
		int in_fd = fileno(input);
		int out_fd = fileno(output);
	
		off_t in_cur = ftell(input);
		off_t out_cur = ftell(output);

		if((inputsize - in_cur) == 0) //There's nothing left to write.
			return;
	
		if(!in_fd || !out_fd)
		{
			fprintf(stderr, "merge:Unable to convert FILE* to fd\n");
			exit(-1);
		}
	
		off_t output_end = out_cur + inputsize - in_cur ;
		if(ftruncate(out_fd, output_end))
		{
			perror("merge: Unable to ftruncute(2) output file:");
			exit(-1);
		}


		off_t input_offset_start = sysconf(_SC_PAGESIZE) * (in_cur / sysconf(_SC_PAGESIZE)); //Round down to nearest page size.
	
		void* input_map = mmap(NULL, inputsize - input_offset_start , PROT_READ, MAP_SHARED, in_fd,  input_offset_start);
		if(input_map == (void*) -1)
		{
			perror( "merge: Unable to mmap input file:");
			exit(-1);
		}

		if(madvise(input_map, inputsize - input_offset_start, MADV_SEQUENTIAL))
		{
			perror("merge: Madvise failed:");
		}
	
		off_t output_offset_start = sysconf(_SC_PAGESIZE) * (out_cur / sysconf(_SC_PAGESIZE));
		void* output_map = mmap(NULL, output_end - output_offset_start   , PROT_WRITE , MAP_SHARED,out_fd,output_offset_start);
	
		if(output_map == (void*) -1)
		{
			munmap(input_map,inputsize-input_offset_start);
			perror("merge: Unable to mmap output file:");
			exit(-1);
		}
	
		memcpy(output_map + (out_cur - output_offset_start ), input_map + (in_cur - input_offset_start ), (inputsize - in_cur ));

		munmap(input_map, inputsize-input_offset_start);
		munmap(output_map,output_end - output_offset_start);
		return;
	}
}
	
//Returns 0 on sucess, non-zero on error.
int merge_files(FILE* in_first,off_t first_size, FILE* in_second,off_t second_size, FILE* out,int max_ngram_string_length)
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
		copy_rest_of_file_to_output(in_second,second_size,out);
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
				copy_rest_of_file_to_output(in_first,first_size,out);
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
				copy_rest_of_file_to_output(in_second,second_size,out);
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
	free(buf);
	free(buf2);
}	
