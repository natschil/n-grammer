#include "util.h"
static int callback_function(const char* filepath,const struct stat *sb, int typeflag,struct FTW* ignore)
{
	remove(filepath);
}

void recursively_remove_directory(const char* directory_name)
{
	nftw(directory_name, &callback_function, 2, FTW_DEPTH);
}
