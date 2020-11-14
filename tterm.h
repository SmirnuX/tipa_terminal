#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>
#include <termios.h>
#include <fcntl.h>
//#include <sighal.h>


#define MAX_LENGTH 512
#define MAX_ARGS 20 
#define MAX_PATH_LENGTH 512
#define MAX_JOBS_COUNT 16
#define PERMISSION 0666





extern int debug_mode, input, output;
extern pid_t jobs[MAX_JOBS_COUNT];
extern char jobs_names[MAX_JOBS_COUNT][MAX_LENGTH];
extern struct termios default_settings, new_settings;

/*
Необходимые фичи:
 - Выполнение команд +
 - Раздвоение +
 - Обработка строк +
 - Закрытие процессов +
 - Демоны и список работ +
 - Обработка ошибок
 
 - Собственная имплементация:
	- cd +
	- jobs +
	- && +
	- | +
	- <
	- > 
*/

//tterm.c - основные подпрограммы
void execute_command(char* command, char** arg_vec, int pipe_in, int pipe_out, int daemon);
void kill_child(int param);
void kill_parent(int param);

//string_parser.c - разбор строки на лексемы
char* new_str_copy(char* source, int beginning, int end);	//Копирует символы из строки source с символа под номером beginning включительно до символа под номером end исключительно. Массив выделяется динамически в результате работы программы. Строка оканчивается нуль-символом. Возвращаемое значение - указатель на подстроку.
char** string_parser(char* string, char* delim);	//Преобразует строку в вектор подстрок, разделенных символами из строки delim. Деление строки происходит с учетом кавычек. Вектор оканчивается NULL-указателем.

//shell_comands.c - Различные команды
void shell_cd(char* path, char** arg_vec);
void shell_jobs();
void shell_kill(char* pid);
