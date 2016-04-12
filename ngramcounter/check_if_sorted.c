#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>

//Check whether or not a file is sorted by line, using the utf8 locale.

int main(int argc, char* argv[])
{

	setlocale(LC_CTYPE,"en_US.UTF-8");
	if(argc != 2)
	{
		fprintf(stderr,"Usage: %s file_name\n",argv[0]);
		return -1;
	}
	const char* filename = argv[1];
	FILE* myfile = fopen(filename,"r");
	if(!myfile)
	{
		fprintf(stderr,"Could not open file %s \n",filename);
		return -1;
	}
	size_t buf1_size = 1024;
	size_t buf2_size = 1024;
	char* buf1 = malloc(buf1_size);
	ssize_t read;
	read = getline(&buf1,&buf1_size,myfile);
	if(read <= 0)
	{
		fprintf(stderr,"The file %s seems to be empty \n",filename);
		return -1;
	}
	if(buf2_size < buf1_size)
	{
		buf2_size = buf1_size;
	}
	char* buf2 = malloc(buf2_size);
	strcpy(buf2,buf1);

	while((read = getline(&buf1,&buf1_size,myfile))> 0)
	{
		if(strcmp(buf1,buf2) <= 0)
		{
			fprintf(stderr,"Failed here: \n %s and %s\n",buf1,buf2);
			return -1;
		}else
		{
			if(buf2_size < buf1_size)
			{
				buf2_size = buf1_size;
				buf2 = realloc(buf2, buf2_size);
			}
			strcpy(buf2,buf1);
		}
	}
	fprintf(stdout,"Success\n");
	return 0;
}
