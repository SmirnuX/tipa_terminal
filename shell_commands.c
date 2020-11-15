#include "tterm.h"

void shell_cd(char* path, char** arg_vec)
{
	chdir(arg_vec[1]);
	getcwd(path,MAX_PATH_LENGTH);
	perror("");
}

void shell_jobs()
{
	for (int i=0; i<MAX_JOBS_COUNT; i++)
	{
		if (strcmp(jobs_names[i], "")!=0)
		{
			printf("[%i] <%i> %s [%s]\n", i, jobs[i], jobs_names[i], (waitpid(jobs[i], NULL, WNOHANG)!=0)?"Closed":"Not closed");
		}
	}
}

void shell_kill(char* pid)
{
	kill(atoi(pid), SIGINT);
}

void shell_help()
{
	printf(	"*****TIPA TERMINAL*****\nBuilt-in commands:\n"
			"cd <directory> - change directory\n"
			"debug - change output to more detailed\n"
			"exit - close terminal\n"
			"jobs - print list of daemons\n"
			"help - print some info about terminal\n"
			"kill <process_id> - close process\n");
}

void shell_exit()
{
	printf("\n");
	exit(0);
}