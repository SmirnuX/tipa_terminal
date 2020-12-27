// SPDX-License-Identifier: CPOL-1.02
#include "tterm.h"
#define _GNU_SOURCE

int debug_mode = -1;	//Включен ли режим отладки
char path[MAX_PATH_LENGTH];	//Путь к текущей директории
pid_t jobs[MAX_JOBS_COUNT];	//pid фоновых процессов
char jobs_names[MAX_JOBS_COUNT][MAX_LENGTH];	//Список названий фоновых процессов
char jobs_closed[MAX_JOBS_COUNT];	//Закрыты ли фоновые процессы


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
	int pipes[3][2];	//Трубы - стандартная, четная и нечетная
	int temp_int = 0;

	pipes[0][0] = STDIN_FILENO;
	pipes[0][1] = STDOUT_FILENO;

	errno = 0;
	if (getcwd(path, MAX_PATH_LENGTH) == NULL)	{
		perror("Getting path error: ");
		return errno;
	}
	if (signal(SIGINT, kill_parent) == SIG_ERR)	{	//Переопределение поведения при полученном сигнале прерывания
		perror("Signal handler error: ");
		return errno;
	}
	for (i = 0; i < MAX_JOBS_COUNT; i++)	{	//Заполняем список демонов пустыми значениями
		strcpy(jobs_names[i], "");
		jobs_closed[i] = 1;
	}
	printf("**** TIPA TERMINAL started*****\n");
	while (1)	{
		printf("%s: > ", path);
		if (fgets(string, MAX_LENGTH, stdin) == NULL)	//Ввод строки
		{
			shell_exit(NULL);
		}
		arg_vec = string_parser(string, "\n ");
		int arg_beg = 0;	//Начало подкоманды
		int arg_count = 0;	//Количество аргументов подкоманды (включая нулевой аргумент)
		int selected_pipe = 0;	//Выбранная труба
		int new_pipe = 0;
		struct IOConfig ioconfig;	//Конфигурация каналов ввода и вывода

		for (i = 0; arg_vec[i] != NULL; i++)	{	//Парсинг строки
			int parsed_string = -1;	//Какая из строк массива parser_string[] найдена
			int in_file, out_file;
			int is_fine = 1;	//Проверка правильности открытия файлов

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
				free_arg_vec(arg_vec);
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
				if (selected_pipe == 0 || selected_pipe == 2)	//Нечетные трубы - чтение из pipe2 (или stdin), вывод в pipe1
					new_pipe = 1;
				else	//Четные трубы - чтение из pipe1, вывод в pipe2
					new_pipe = 2;
				if (pipe(pipes[new_pipe]) == -1)	{
					perror("Pipe creating error: ");
					free_arg_vec(arg_vec);
					free(temp_vec);
					return errno;
				}
				ioconfig.in_desc = pipes[selected_pipe][0];
				ioconfig.out_desc = pipes[new_pipe][1];
				selected_pipe = new_pipe;
				break;
			case 1:	// '<' - ввод из файла
				if (arg_vec[i+1] == NULL)	{
					printf("Missing filename after \'<\'\n");
					is_fine = 0;
				}
				in_file = open(arg_vec[i+1], O_RDONLY);
				if (in_file == -1)	{
					perror("Input file error: ");
					is_fine = 0;
				}
				i++;
				ioconfig.in_desc = in_file;
				ioconfig.is_file_in = 1;
				break;
			case 2:	// '>' - вывод в файл
				if (arg_vec[i+1] == NULL)	{
					printf("Missing filename after \'>\'\n");
					is_fine = 0;
				}
				out_file = open(arg_vec[i+1], O_CREAT | O_WRONLY, PERMISSION);
				if (out_file == -1)	{
					perror("Output file error: ");
					is_fine = 0;
				}
				i++;
				ioconfig.out_desc = out_file;
				ioconfig.is_file_out = 1;
				break;
			}
			if (parsed_string >= 2 && parsed_string <= 3)	{	//'&&' - обработка последовательных команд
				ioconfig.in_desc = pipes[selected_pipe][0];
				selected_pipe = 0;
			}
			if (is_fine == 1)
				execute_command(command, temp_vec, ioconfig, 0);
			switch (parsed_string)	{	//Закрытие файлов
			case 1:
				close(in_file);
				break;
			case 2:
				close(out_file);
				break;
			}
			arg_vec[arg_beg] = command;
			free(temp_vec);
			arg_beg = i+1;
		}
		//Выполнение последней команды
		if (arg_vec[arg_beg] != NULL)	{
			int is_daemon = 0;

			if (strcmp("&", arg_vec[i-1]) == 0)	{	//Обработка демонов
				is_daemon = 1;
				arg_vec[i-1] = NULL;
				check_daemons();
				for (i = 0; i < MAX_JOBS_COUNT; i++)	{
					if (jobs_closed[i] == 1)	{	//Проверка на уже закрытые фоновые процессы или еще не открытые
						strncpy(jobs_names[i], string, MAX_LENGTH);
						jobs_closed[i] = 0;
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
			ioconfig.in_desc = pipes[selected_pipe][0];
			ioconfig.out_desc = STDOUT_FILENO;
			ioconfig.is_file_in = 0;
			ioconfig.is_file_out = 0;
			execute_command(command, arg_vec + arg_beg, ioconfig, is_daemon*(i+1));
			arg_vec[arg_beg] = command;
		}
		free_arg_vec(arg_vec);
	}
	return 0;
}

void execute_command(char *command, char **arg_vec, struct IOConfig ioconfig, int daemon)
{
	//Проверка на встроенные команды
	for (int i = 0; i < CMD_COUNT; i++)	{
		if (strcmp(command, shell_cmd[i]) == 0)	{
			shell_cmd_ptrs[i](arg_vec);
			return;
		}
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
			if (daemon != 0)
				jobs_closed[daemon - 1] = 1;
			arg_vec[0] = command;
			free_arg_vec(arg_vec);
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
			jobs_closed[daemon-1] = 0;
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
		shell_exit(NULL);
}

void check_daemons(void)	//Обновление статусов демонов
{
	int status;

	for (int i = 0; i < MAX_JOBS_COUNT; i++)	{
		if (strcmp(jobs_names[i], "") == 0 || jobs_closed[i] == 1)
			continue;
		pid_t return_pid = waitpid(jobs[i], &status, WNOHANG);

		if (return_pid == -1)
			perror("Daemon check error: ");
		else if (return_pid == 0)
			jobs_closed[i] = 0;
		else if (return_pid == jobs[i])
			jobs_closed[i] = 1;
	}
}

void free_arg_vec(char **arg_vec)
{
	for (int i = 0; arg_vec[i] != NULL; i++)
		free(arg_vec[i]);
	free(arg_vec);
}
