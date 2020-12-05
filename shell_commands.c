// SPDX-License-Identifier: CPOL-1.02
#include "tterm.h"

char *shell_cmd[CMD_COUNT] = {"cd", "debug", "exit", "jobs", "help", "kill"};
void (*shell_cmd_ptrs[CMD_COUNT])(char **) = {shell_cd, shell_debug, shell_exit, shell_jobs, shell_help, shell_kill};




void shell_cd(char **arg_vec)	//Переход в другую директорию
{
	if (arg_vec[1] == NULL)	{
		printf("This program needs one argument to work.\n");
		return;
	}
	if (chdir(arg_vec[1]) == -1)	{
		perror("");
		return;
	}
	if (debug_mode == 1)
		printf("Changed directory to %s\n", arg_vec[1]);
	getcwd(path, MAX_PATH_LENGTH);
}

void shell_jobs(char **arg_vec)	//Вывод списка демонов
{
	check_daemons();
	for (int i = 0; i < MAX_JOBS_COUNT; i++)	{
		if (strcmp(jobs_names[i], "") != 0)
			printf("[%i] <%i> %s [%s]\n", i, jobs[i], jobs_names[i], (jobs_closed[i] == 1)?"Closed":"Not closed");
	}
}

void shell_kill(char **arg_vec)	//Закрытие процесса
{
	if (arg_vec[1] == NULL)	{
		printf("This program needs one argument to work.\n");
		return;
	}
	kill(atoi(arg_vec[1]), SIGINT);
	waitpid(atoi(arg_vec[1]), NULL, 0);
}

void shell_help(char **arg_vec)	//Вывод небольшой справки
{
	printf("*****TIPA TERMINAL*****\nBuilt-in commands:\n"
			"cd <directory> - change directory\n"
			"debug - change output to more detailed\n"
			"exit - close terminal\n"
			"jobs - print list of daemons\n"
			"help - print some info about terminal\n"
			"kill <process_id> - close process\n\n"
			"Also this terminal have support of:\n"
			"& in the end of string - launch last command as daemon\n"
			"< <filename> - command before \"<\" will get input from <filename>\n"
			"> <filename> - command before \">\" will give output to <filename>\n"
			"&& - commands after \"&&\" will be executed after command before ends\n"
			"| - output from command before \"|\" will go to input of command after \"|\"\n");
}

void shell_exit(char **arg_vec)	//Закрытие терминала
{
	printf("\n");
	exit(0);
}

void shell_debug(char **arg_vec)
{
	if (debug_mode == -1) {
		printf("Debug mode ON\n");
		debug_mode = 1;
	} else {
		printf("Debug mode OFF\n");
		debug_mode = -1;
	}
}
