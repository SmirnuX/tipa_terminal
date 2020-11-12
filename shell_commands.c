#include "tterm.h"

int shell_cd(char* path, char** arg_vec)
{
	chdir(arg_vec[1]);
	getcwd(path,MAX_PATH_LENGTH);
	perror("");
}