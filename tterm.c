#include "tterm.h"
#define _GNU_SOURCE

int debug_mode=0;
char path[MAX_PATH_LENGTH];
pid_t jobs[MAX_JOBS_COUNT];
char jobs_names[MAX_JOBS_COUNT][MAX_LENGTH];

int main(int argc, char *argv[])
{
	char string[MAX_LENGTH];
	char* command;
	char **arg_vec;
	int arg_count;
	char tmp;
	int i, j;
	char is_word;
	int is_in_bracket=0;
	int selected_pipe=0;
	char** temp_vec;
	int pipe1[2], pipe2[2];
	getcwd(path,MAX_PATH_LENGTH);
	signal(SIGINT, kill_parent);
	int fpipe[2];
	int tempfile = open(".termtmp", O_CREAT|O_WRONLY, S_IWUSR);

	for(i=0; i<MAX_JOBS_COUNT; i++)	//Заполняем список демонов пустыми значениями
	{
		strcpy(jobs_names[i], "");
	}
	while(1)
	{
		printf("%s: > ",path);
		fgets(string, MAX_LENGTH, stdin);	//Ввод строки
		arg_vec = string_parser(string, " \n");
		i = 0;
		arg_count=0;
		int parts=1;
		int arg_beg=0;
		int is_daemon=0;
		selected_pipe=0;
		//Парсим на предмет труб, демонов и выводов / вводов из файла
		while(arg_vec[i]!=NULL)
		{
			if (strcmp(arg_vec[i], "|")==0 || strcmp(arg_vec[i], ">")==0 ||strcmp(arg_vec[i], "&&")==0)
			{
				temp_vec=malloc(sizeof(char*)*(arg_count+1));
				for (j=0; j<arg_count; j++)
				{
					temp_vec[j]=arg_vec[arg_beg+j];
				}
				temp_vec[arg_count]=NULL;
				arg_count=0;
				arg_beg=i+1;
				command=temp_vec[0];
				temp_vec[0]=path;
				if (strcmp(arg_vec[i], "|")==0)
				{
					switch (selected_pipe)
					{
						case 0:	//Первая труба - перенаправление вывода в pipe1
							pipe(pipe1);
							execute_command(command, temp_vec, STDIN_FILENO, pipe1[1], -1);
							selected_pipe=1;
							break;
						case 1: //Четные трубы - чтение из pipe1, вывод в pipe2
							pipe(pipe2);
							execute_command(command, temp_vec, pipe1[0], pipe2[1], -1);
							selected_pipe=2;
							break;
						case 2: //Нечетные трубы - чтение из pipe2, вывод в pipe1
							pipe(pipe1);
							execute_command(command, temp_vec, pipe2[0], pipe1[1], -1);
							selected_pipe=1;
							break;
					}
				}
				else if (strcmp(arg_vec[i], "&&")==0)
				{
					switch (selected_pipe)
					{
						case 0:	//Первая труба - перенаправление вывода в pipe1
							pipe(pipe1);
							execute_command(command, temp_vec, STDIN_FILENO, STDOUT_FILENO, -1);
							break;
						case 1: //Четные трубы - чтение из pipe1, вывод в pipe2
							execute_command(command, temp_vec, pipe1[0], STDOUT_FILENO, -1);
							selected_pipe=0;
							break;
						case 2: //Нечетные трубы - чтение из pipe2, вывод в pipe1
							execute_command(command, temp_vec, pipe2[0], STDOUT_FILENO, -1);
							selected_pipe=0;
							break;
					}
				}
				else if (strcmp(arg_vec[i], ">")==0)
				{
					int out_file = open(arg_vec[i+1], O_CREAT, PERMISSION);
					perror("");
					//TODO - проверить на отсутствие i+1 и на отсутствие файла
					switch (selected_pipe)
					{
						case 0:	//Нет трубы
							execute_command(command, temp_vec, STDIN_FILENO, out_file, -1);
							break;
						case 1: 
							execute_command(command, temp_vec, pipe1[0], out_file, -1);
							selected_pipe=0;
							break;
						case 2: //Нечетные трубы - чтение из pipe2, вывод в pipe1
							execute_command(command, temp_vec, pipe2[0], out_file, -1);
							selected_pipe=0;
							break;
					}
					//close(out_file);
				}
				free(temp_vec);
			}
			else arg_count++;
			i++;
		}
		//Выполнение последней команды
		//Поставить парсинг демонов
		if (strcmp("&",arg_vec[i-1])==0)
		{
			is_daemon=1;
			arg_vec[i-1]=NULL;
			for(i=0; i<MAX_JOBS_COUNT; i++)
			{
				//Проверка на уже закрытые фоновые процессы или еще не открытые
				if (waitpid(jobs[i], NULL, WNOHANG)!=0 || strcmp(jobs_names[i],"")==0)
				{
					strncpy(jobs_names[i], string, MAX_LENGTH);
					break;
				}
			}
			if (i == MAX_JOBS_COUNT)
			{
				printf("ERROR: There are too many running daemons already\n");
				is_daemon = 0;
			}
		}
		command=arg_vec[arg_beg];
		arg_vec[arg_beg]=path;
		switch (selected_pipe)
		{
			case 0:	//Команда без труб
				execute_command(command, arg_vec, STDIN_FILENO, STDOUT_FILENO, is_daemon*i);
				selected_pipe=1;
				break;
			case 1: //Четная труба
				execute_command(command, arg_vec + arg_beg, pipe1[0], STDOUT_FILENO, is_daemon*i);
				selected_pipe=2;
				break;
			case 2: //Нечетные трубы - чтение из pipe2, вывод в pipe1
				execute_command(command, arg_vec + arg_beg, pipe2[0], STDOUT_FILENO, is_daemon*i);
				selected_pipe=1;
				break;
		}
		free(arg_vec);
    }
	return 0;
}

void kill_child(int param)
{
	if (debug_mode==1)
		printf("\nClosed\n");
	else
		printf("\n");	
	signal(SIGINT, kill_parent);
}

void kill_parent(int param)
{
	printf("\n");
	exit(0);
}

void execute_command(char* command, char** arg_vec, int pipe_in, int pipe_out, int daemon)
{
	//Проверка на встроенные команды
	if (strcmp(command, "cd")==0)
	{
		if (debug_mode==1)
			printf("Change directory to %s\n", arg_vec[1]);
		shell_cd(path, arg_vec);
		return;
	}
	else if (strcmp(command, "jobs")==0)
	{
		shell_jobs();
		return;
	}
	else if (strcmp(command, "kill")==0)
	{
		if (arg_vec[1]==NULL)
		{
			printf("There are no arguments\n");
			return;
		}
		shell_kill(arg_vec[1]);
		return;
	}
	else if (strcmp(command, "debug")==0)
	{
		if (debug_mode==0)
		{
			printf("Debug mode ON\n");
			debug_mode=1;
		}
		else
		{
			printf("Debug mode OFF\n");
			debug_mode=0;
		}
		return;	
	}
	//Раздвоение
	int child=fork();
	int child_state;
	switch (child)
	{
		case 0:
			if (pipe_in!=STDIN_FILENO)
			{
				close(STDIN_FILENO);
				dup(pipe_in);
			}
			if (pipe_out!=STDOUT_FILENO)
			{
				close(STDOUT_FILENO);
				dup(pipe_out);
				perror("");
			}
			if (execvp(command, arg_vec)==-1)
			{
				perror("");
			}
			return;
		case -1:
			printf("ERROR 404 - бляздец\n");
			break;
		default:
			if (pipe_in!=STDIN_FILENO)
				close(pipe_in);
			if (pipe_out!=STDOUT_FILENO)
				close(pipe_out);
			signal(SIGINT, kill_child);	//Перехватываем сигнал, идущий в предка
			if (daemon==-1)
				waitpid(child, &child_state, 0);
			else if (daemon<MAX_JOBS_COUNT)
				jobs[daemon]=child;
			signal(SIGINT, kill_parent);	//Возвращаем стандартное поведение при сигнале прерывания
			break;
	}
}
