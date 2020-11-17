// SPDX-License-Identifier: CPOL-1.02
#include "tterm.h"
#define _GNU_SOURCE

int debug_mode = -1;	//Включен ли режим отладки
char path[MAX_PATH_LENGTH];
pid_t jobs[MAX_JOBS_COUNT];
char jobs_names[MAX_JOBS_COUNT][MAX_LENGTH];
char jobs_closed[MAX_JOBS_COUNT];

int main(int argc, char *argv[])
{
	const char *parser_strings[5] = {"|", "<", ">", "&&", NULL};	//Комбинации символов, которые требуют дополнительной обработки
	char string[MAX_LENGTH];
	char *command;
	char **arg_vec;
	char tmp;
	int i, j;
	char is_word;
	int is_in_bracket = 0;
	char **temp_vec;
	int pipe1[2], pipe2[2];
	int fpipe[2];
	int temp_int = 0;

	errno = 0;
	if (getcwd(path, MAX_PATH_LENGTH) == NULL)	{
		perror("Getting path error: ");
		return errno;
	}
	if (signal(SIGINT, kill_parent) == SIG_ERR)	{	//Переопределение поведения при полученном сигнале прерывания
		perror("Signal handler error: ");
		return errno;
	}
	for (i = 0; i < MAX_JOBS_COUNT; i++)	//Заполняем список демонов пустыми значениями
		strcpy(jobs_names[i], "");
	printf("**** TIPA TERMINAL started*****\n");
	while (1)	{
		printf("%s: > ", path);
		fgets(string, MAX_LENGTH, stdin);	//Ввод строки
		arg_vec = string_parser(string, "\n ");
		int arg_beg = 0;	//Начало подкоманды
		int arg_count = 0;	//Количество аргументов подкоманды (включая нулевой аргумент)
		int selected_pipe = 0;	//Выбранная труба
		struct IOConfig ioconfig;	//Конфигурация каналов ввода и вывода

		for (i = 0; arg_vec[i] != NULL; i++)	{	//Парсинг строки
			int parsed_string = -1;	//Какая из строк массива parser_string[] найдена
			int in_file, out_file;

			for (j = 0; parser_strings[j] != NULL; j++)	{
				if (strcmp(arg_vec[i], parser_strings[j]) == 0)	{
					parsed_string = j;
					break;
				}
			}
			if (parsed_string < 0)	{
				arg_count++;
				continue;
			}
			//Выделение подвектора вектора аргументов
			temp_vec = malloc(sizeof(char *) * (arg_count+1));
			if (temp_vec == NULL)	{
				perror("Memory allocation error: ");
				free(arg_vec);
				return errno;
			}
			for (j = 0; j < arg_count; j++)
				temp_vec[j] = arg_vec[arg_beg+j];
			temp_vec[arg_count] = NULL;	//"Закрытие" подвектора
			arg_count = 0;
			command = temp_vec[0];
			temp_vec[0] = path;
			//Обнуление конфигурации ввода/вывода
			ioconfig.in_desc = STDIN_FILENO;
			ioconfig.out_desc = STDOUT_FILENO;
			ioconfig.is_file_in = 0;
			ioconfig.is_file_out = 0;
			switch (parsed_string)	{	//Обработка труб
			case 0:	// '|' - трубы
				switch (selected_pipe)	{
				case 0:	//Первая труба - перенаправление вывода в pipe1
					if (pipe(pipe1) == -1)	{
						perror("Pipe creating error: ");
						free(arg_vec);
						free(temp_vec);
						return errno;
					}
					ioconfig.out_desc = pipe1[1];
					selected_pipe = 1;
					break;
				case 1: //Четные трубы - чтение из pipe1, вывод в pipe2
					if (pipe(pipe2) == -1)	{
						perror("Pipe creating error: ");
						free(arg_vec);
						free(temp_vec);
						return errno;
					}
					ioconfig.in_desc = pipe1[0];
					ioconfig.out_desc = pipe2[1];
					selected_pipe = 2;
					break;
				case 2: //Нечетные трубы - чтение из pipe2, вывод в pipe1
					if (pipe(pipe1) == -1)	{
						perror("Pipe creating error: ");
						free(arg_vec);
						free(temp_vec);
						return errno;
					}
					ioconfig.in_desc = pipe2[0];
					ioconfig.out_desc = pipe1[1];
					selected_pipe = 1;
					break;
				}
				break;
			case 1:	// '<' - ввод из файла
				if (arg_vec[i+1] == NULL)	{
					printf("Missing filename after \'<\'\n");
					break;
				}
				in_file = open(arg_vec[i+1], O_RDONLY);
				if (in_file == -1)	{
					perror("Input file error: ");
					break;
				}
				i++;
				ioconfig.in_desc = in_file;
				ioconfig.is_file_in = 1;
				break;
			case 2:	// '>' - вывод в файл
				if (arg_vec[i+1] == NULL)	{
					printf("Missing filename after \'>\'\n");
					break;
				}
				out_file = open(arg_vec[i+1], O_CREAT | O_WRONLY, PERMISSION);
				if (in_file == -1)	{
					perror("Output file error: ");
					break;
				}
				i++;
				ioconfig.out_desc = out_file;
				ioconfig.is_file_out = 1;
				break;
			}
			if (parsed_string >= 1 && parsed_string <= 3)	{	//'&&' - обработка последовательных команд
				switch (selected_pipe)	{
				case 1: //Четные трубы - чтение из pipe1
					ioconfig.in_desc = pipe1[0];
					break;
				case 2: //Нечетные трубы - чтение из pipe2
					ioconfig.in_desc = pipe2[0];
					break;
				}
				selected_pipe = 0;
			}
			execute_command(command, temp_vec, ioconfig, 0);
			switch (parsed_string)	{	//Закрытие файлов
			case 1:
				close(in_file);
				break;
			case 2:
				close(out_file);
				break;
			}
			free(temp_vec);
			arg_beg = i+1;
		}
		//Выполнение последней команды
		if (arg_vec[arg_beg] != NULL)	{
			int is_daemon = 0;

			if (strcmp("&", arg_vec[i-1]) == 0)	{	//Обработка демонов
				is_daemon = 1;
				arg_vec[i-1] = NULL;
				for (i = 0; i < MAX_JOBS_COUNT; i++)	{
					int ch_status, wp_status;

					wp_status = waitpid(jobs[i], &ch_status, WNOHANG);
					//Проверка на уже закрытые фоновые процессы или еще не открытые
					if (WIFEXITED(ch_status) * wp_status != 0 || strcmp(jobs_names[i], "") == 0)	{
						strncpy(jobs_names[i], string, MAX_LENGTH);
						break;
					}
				}
				if (i == MAX_JOBS_COUNT)	{
					printf("ERROR: There are too many running daemons already\n");
					is_daemon = 0;
				}
			}
			command = arg_vec[arg_beg];
			arg_vec[arg_beg] = path;
			//Обнуление конфигурации ввода/вывода
			ioconfig.in_desc = STDIN_FILENO;
			ioconfig.out_desc = STDOUT_FILENO;
			ioconfig.is_file_in = 0;
			ioconfig.is_file_out = 0;
			switch (selected_pipe)	{
			case 0:	//Команда без труб
				execute_command(command, arg_vec + arg_beg, ioconfig, is_daemon*(i+1));
				selected_pipe = 1;
				break;
			case 1: //Четная труба
				ioconfig.in_desc = pipe1[0];
				execute_command(command, arg_vec + arg_beg, ioconfig, is_daemon*(i+1));
				selected_pipe = 2;
				break;
			case 2: //Нечетные трубы - чтение из pipe2, вывод в pipe1
				ioconfig.in_desc = pipe2[0];
				execute_command(command, arg_vec + arg_beg, ioconfig, is_daemon*(i+1));
				selected_pipe = 1;
				break;
			}
		}
		free(arg_vec);
	}
	return 0;
}

void execute_command(char *command, char **arg_vec, struct IOConfig ioconfig, int daemon)
{
	//Проверка на встроенные команды
	if (strcmp(command, "cd") == 0)	{
		shell_cd(arg_vec);
		return;
	}	else if (strcmp(command, "jobs") == 0)	{
		shell_jobs();
		return;
	}	else if (strcmp(command, "kill") == 0)	{
		if (arg_vec[1] == NULL)	{
			printf("There are no arguments\n");
			return;
		}
		shell_kill(arg_vec[1]);
		return;
	}	else if (strcmp(command, "debug") == 0)	{
		shell_debug();
		return;
	}	else if (strcmp(command, "help") == 0)	{
		shell_help();
		return;
	}	else if (strcmp(command, "exit") == 0)	{
		shell_exit();
		return;
	}
	//Раздвоение
	int child = fork();
	int child_state;

	switch (child)	{
	case 0:	//Потомок
		if (ioconfig.in_desc != STDIN_FILENO)	{
			close(STDIN_FILENO);
			dup(ioconfig.in_desc);
		}
		if (ioconfig.out_desc != STDOUT_FILENO)	{
			close(STDOUT_FILENO);
			dup(ioconfig.out_desc);
		}
		if (execvp(command, arg_vec) == -1)	{
			perror("Execution error: ");
			exit(errno);
		}
		return;
	case -1:	//Ошибка при раздвоении
		perror("Fork error: ");
		break;
	default:	//Предок
		if (ioconfig.in_desc != STDIN_FILENO)
			close(ioconfig.in_desc);
		if (ioconfig.out_desc != STDOUT_FILENO)
			close(ioconfig.out_desc);
		if (daemon == 0)	{
			signal(SIGINT, kill_child);	//Перехватываем сигнал, идущий в предка
			waitpid(child, NULL, 0);
			signal(SIGINT, kill_parent);	//Возвращаем стандартное поведение при сигнале прерывания
		}	else if (daemon < MAX_JOBS_COUNT+1)	{
			jobs[daemon-1] = child;
			if (debug_mode == 1)
				printf("Created daemon #%i %s with pid [%i]\n", daemon, jobs_names[daemon-1], jobs[daemon-1]);
		}
		break;
	}
}

void kill_child(int param)	//Обработка сигнала для закрытия потомка
{
	signal(SIGINT, kill_parent);
}

void kill_parent(int param)	//Обработка сигнала для закрытия предка (если EXIT_ON_SIGNAL = 1)
{
	if (EXIT_ON_SIGNAL == 1)
		shell_exit();
}
