#include "tterm.h"
#define _GNU_SOURCE

int debug_mode=0;
char path[MAX_PATH_LENGTH];
pid_t jobs[MAX_JOBS_COUNT];
char jobs_names[MAX_JOBS_COUNT][MAX_LENGTH];

int main(int argc, char *argv[])
{
	const char* parser_strings[] = {"|","<",">","&&", NULL};	//Комбинации символов, которые требуют дополнительной обработки
	char string[MAX_LENGTH];
	char* command;
	char **arg_vec;
	char tmp;
	int i, j;
	char is_word;
	int is_in_bracket=0;
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
		int arg_count=0;
		int parts=1;
		int arg_beg=0;
		
		int selected_pipe=0;
		struct IOConfig ioconfig;
		//Ввод и вывод по умолчанию
		
		while(arg_vec[i]!=NULL)	//Парсинг строки
		{
			if (strcmp(arg_vec[i], "|")==0 || strcmp(arg_vec[i], ">")==0 ||strcmp(arg_vec[i], "&&")==0 || strcmp(arg_vec[i], "<")==0)
			{
				//Выделение подвектора вектора аргументов
				temp_vec=malloc(sizeof(char*)*(arg_count+1));	
				for (j=0; j<arg_count; j++)
				{
					temp_vec[j]=arg_vec[arg_beg+j];
				}
				temp_vec[arg_count]=NULL;	//"Закрытие" подвектора
				arg_count=0;
				arg_beg=i+1;
				command=temp_vec[0];
				temp_vec[0]=path;
				//Обнуление конфигурации ввода/вывода
				ioconfig.in_desc=STDIN_FILENO;
				ioconfig.out_desc=STDOUT_FILENO;
				ioconfig.is_file_in=0;
				ioconfig.is_file_out=0;
				if (strcmp(arg_vec[i], "|")==0)	//Обработка труб
				{
					switch (selected_pipe)
					{
						case 0:	//Первая труба - перенаправление вывода в pipe1
							pipe(pipe1);
							ioconfig.out_desc=pipe1[1];
							selected_pipe=1;
							break;
						case 1: //Четные трубы - чтение из pipe1, вывод в pipe2
							pipe(pipe2);
							ioconfig.in_desc=pipe1[0];
							ioconfig.out_desc=pipe2[1];
							selected_pipe=2;
							break;
						case 2: //Нечетные трубы - чтение из pipe2, вывод в pipe1
							pipe(pipe1);
							ioconfig.in_desc=pipe2[0];
							ioconfig.out_desc=pipe1[1];
							selected_pipe=1;
							break;
					}
					execute_command(command, temp_vec, ioconfig, 0);
				}
				else if (strcmp(arg_vec[i], "&&")==0)	//Обработка последовательных команд
				{
					switch (selected_pipe)
					{
						case 1: //Четные трубы - чтение из pipe1
							ioconfig.in_desc=pipe1[0];
							selected_pipe=0;
							break;
						case 2: //Нечетные трубы - чтение из pipe2
							ioconfig.in_desc=pipe2[0];
							selected_pipe=0;
							break;
					}
					execute_command(command, temp_vec, ioconfig, 0);
				}
				else if (strcmp(arg_vec[i], ">")==0)	//Вывод в файл
				{
					int out_file = open(arg_vec[i+1], O_CREAT | O_WRONLY, PERMISSION);
					//TODO - check for i+1 == NULL
					i++;
					arg_beg=i+1;
					ioconfig.out_desc=out_file;
					ioconfig.is_file_out=1;
					switch (selected_pipe)
					{
						case 1: 
							ioconfig.in_desc=pipe1[0];
							selected_pipe=0;
							break;
						case 2: 
							ioconfig.in_desc=pipe2[0];
							selected_pipe=0;
							break;
					}
					execute_command(command, temp_vec, ioconfig, 0);
					close(out_file);
				}
				else if (strcmp(arg_vec[i], "<")==0)	//Вывод в файл
				{
					int in_file = open(arg_vec[i+1], O_RDONLY);
					//TODO - check for i+1 == NULL
					i++;
					arg_beg=i+1;
					ioconfig.in_desc=in_file;
					ioconfig.is_file_in=1;
					switch (selected_pipe)
					{
						case 1: 
							ioconfig.in_desc=pipe1[0];
							selected_pipe=0;
							break;
						case 2: 
							ioconfig.in_desc=pipe2[0];
							selected_pipe=0;
							break;
					}
					execute_command(command, temp_vec, ioconfig, 0);
					close(in_file);
				}
				free(temp_vec);
			}
			else arg_count++;
			i++;
		}
		//Выполнение последней команды
		if (arg_vec[arg_beg]!=NULL)
		{
			int is_daemon=0;
			if (strcmp("&",arg_vec[i-1])==0)	//Обработка демонов
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
			//Обнуление конфигурации ввода/вывода
			ioconfig.in_desc=STDIN_FILENO;
			ioconfig.out_desc=STDOUT_FILENO;
			ioconfig.is_file_in=0;
			ioconfig.is_file_out=0;
			switch (selected_pipe)
			{
				case 0:	//Команда без труб
					execute_command(command, arg_vec, ioconfig, is_daemon*(i+1));
					selected_pipe=1;
					break;
				case 1: //Четная труба
					ioconfig.in_desc=pipe1[0];
					execute_command(command, arg_vec + arg_beg, ioconfig, is_daemon*(i+1));
					selected_pipe=2;
					break;
				case 2: //Нечетные трубы - чтение из pipe2, вывод в pipe1
					ioconfig.in_desc=pipe2[0];
					execute_command(command, arg_vec + arg_beg, ioconfig, is_daemon*(i+1));
					selected_pipe=1;
					break;
			}
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
	if (EXIT_ON_SIGNAL == 1)
		shell_exit();
}

void execute_command(char* command, char** arg_vec, struct IOConfig ioconfig, int daemon)
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
	else if (strcmp(command, "help")==0)
	{
		shell_help();
		return;
	}
	else if (strcmp(command, "exit")==0)
	{
		shell_exit();
		return;
	}
	//Раздвоение
	int child=fork();
	int child_state;
	switch (child)
	{
		case 0:	//Предок
			if (ioconfig.in_desc!=STDIN_FILENO)
			{
				close(STDIN_FILENO);
				dup(ioconfig.in_desc);
			}
			if (ioconfig.out_desc!=STDOUT_FILENO)
			{
				close(STDOUT_FILENO);
				dup(ioconfig.out_desc);
			}
			if (execvp(command, arg_vec)==-1)
			{

			}
			return;
		case -1:	//Ошибка при раздвоении
			printf("ERROR 404 - бляздец\n");
			break;
		default:	//Прардитель
			if (ioconfig.in_desc!=STDIN_FILENO)
				close(ioconfig.in_desc);
			if (ioconfig.out_desc!=STDOUT_FILENO)
				close(ioconfig.out_desc);
			signal(SIGINT, kill_child);	//Перехватываем сигнал, идущий в предка
			if (daemon==0)
			{
				waitpid(child, NULL, 0);
				printf("Ende");
			}
			else if (daemon<MAX_JOBS_COUNT+1)
			{
				if (debug_mode==1)
					printf("Created daemon #%i %s with pid [%i]\n", daemon, jobs_names[daemon-1], child);
				jobs[daemon-1]=child;
			}
			signal(SIGINT, kill_parent);	//Возвращаем стандартное поведение при сигнале прерывания
			break;
	}
}
