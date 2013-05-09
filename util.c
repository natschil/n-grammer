#include "util.h"
static int callback_function(const char* filepath,const struct stat *ignore_one, int ignore_two,struct FTW* ignore_three)
{
	remove(filepath);
	return 0;
}

void recursively_remove_directory(const char* directory_name)
{
	nftw(directory_name, &callback_function, 2, FTW_DEPTH);
}
