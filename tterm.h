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

//******Коды клавиш******
#define CTRL_C		0x3



extern int debug_mode, input, output;
extern struct termios default_settings, new_settings;

/*
Необходимые фичи:
 - Выполнение команд +
 - Раздвоение +
 - Обработка строк +
 - Закрытие процессов +
 - "Трубы" и вывод в файлы
 - Демоны и список работ
 - Обработка ошибок
 - предыдущие/следующие команды (?)
 - Управление - перемещение влево/вправо по строке, (?)
 
 - Собственная имплементация:
	- cd +
	- jobs
	- exit
	- &&
	- |
*/

//tterm.c - основные подпрограммы
void execute_command(char* command, char** arg_vec, int pipe_in, int pipe_out);
void kill_child(int param);
void kill_parent(int param);

//string_parser.c - разбор строки на лексемы
char* new_str_copy(char* source, int beginning, int end);	//Копирует символы из строки source с символа под номером beginning включительно до символа под номером end исключительно. Массив выделяется динамически в результате работы программы. Строка оканчивается нуль-символом. Возвращаемое значение - указатель на подстроку.
char** string_parser(char* string, char* delim);	//Преобразует строку в вектор подстрок, разделенных символами из строки delim. Деление строки происходит с учетом кавычек. Вектор оканчивается NULL-указателем.

//shell_comands.c - Различные команды
int shell_cd(char* path, char** arg_vec);
