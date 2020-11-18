// SPDX-License-Identifier: CPOL-1.02
#include "tterm.h"

void shell_cd(char **arg_vec)	//Переход в другую директорию
{
	if (debug_mode == 1)
		printf("Changed directory to %s\n", arg_vec[1]);
	if (chdir(arg_vec[1]) == -1)
		perror("");
	getcwd(path, MAX_PATH_LENGTH);
}

void shell_jobs(void)	//Вывод списка демонов
{
	check_daemons();
	for (int i = 0; i < MAX_JOBS_COUNT; i++)	{
		if (strcmp(jobs_names[i], "") != 0)
			printf("[%i] <%i> %s [%s]\n", i, jobs[i], jobs_names[i], (jobs_closed[i] == 1)?"Closed":"Not closed");
	}
}

void shell_kill(char *pid)	//Закрытие процесса
{
	kill(atoi(pid), SIGINT);
}

void shell_help(void)	//Вывод небольшой справки
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

void shell_exit(void)	//Закрытие терминала
{
	printf("\n");
	exit(0);
}

void shell_debug(void)
{
	if (debug_mode == -1) {
		printf("Debug mode ON\n");
		debug_mode = 1;
	} else {
		printf("Debug mode OFF\n");
		debug_mode = -1;
	}
}
