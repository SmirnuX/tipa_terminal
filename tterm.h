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
#include <errno.h>

#define MAX_LENGTH 512
#define MAX_ARGS 20 
#define MAX_PATH_LENGTH 512
#define MAX_JOBS_COUNT 16
#define PERMISSION 0666
#define EXIT_ON_SIGNAL 1

extern int debug_mode;
extern char path[MAX_PATH_LENGTH];
extern pid_t jobs[MAX_JOBS_COUNT];
extern char jobs_closed[MAX_JOBS_COUNT];
extern char jobs_names[MAX_JOBS_COUNT][MAX_LENGTH];
extern struct termios default_settings, new_settings;

struct IOConfig
{
	int in_desc;	//Канал ввода
	int out_desc;	//Канал вывода
	int is_file_in;	//Является ли канал ввода файлом?
	int is_file_out;	//Является ли канал вывода файлом?
};

//tterm.c - основные подпрограммы
void execute_command(char* command, char** arg_vec, struct IOConfig ioconfig, int daemon);
void kill_child(int param);	//Обработка сигнала для закрытия потомка
void kill_parent(int param);	//Обработка сигнала для закрытия предка (если EXIT_ON_SIGNAL = 1) 
void check_daemons(void);	//Обновление статусов демонов
void free_arg_vec(char **arg_vec);	//Очистка памяти

//string_parser.c - разбор строки на лексемы
char* new_str_copy(char* source, int beginning, int end);	//Копирует символы из строки source с символа под номером beginning включительно до символа под номером end исключительно. Массив выделяется динамически в результате работы программы. Строка оканчивается нуль-символом. Возвращаемое значение - указатель на подстроку.
char** string_parser(char* string, char* delim);	//Преобразует строку в вектор подстрок, разделенных символами из строки delim. Деление строки происходит с учетом кавычек. Вектор оканчивается NULL-указателем.

//shell_comands.c - Различные команды
void shell_cd(char** arg_vec);	//Переход в другую директорию
void shell_jobs(void);	//Вывод списка демонов
void shell_kill(char* pid);	//Закрытие процесса
void shell_help(void);	//Вывод небольшой справки
void shell_exit(void);	//Закрытие терминала
void shell_debug(void);