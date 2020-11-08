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

//******Коды клавиш******
#define CTRL_C		0x3



extern int debug_mode, input, output;
extern struct termios default_settings, new_settings;

/*
Необходимые фичи:
 - Выполнение команд +
 - Раздвоение +
 - Обработка строк +
 - предыдущие/следующие команды (?)
 - Управление - перемещение влево/вправо по строке, (?)
 - Закрытие процессов
 - "Трубы" и вывод в файлы
 - Демоны и список работ
 - Обработка ошибок
 - Команды cd и exit
 
 - Собственная имплементация:
	- cd
	- jobs
	- exit
	- &&
	- |
*/

char* new_str_copy(char* source, int beginning, int end);	//Копирует символы из строки source с символа под номером beginning включительно до символа под номером end исключительно. Массив выделяется динамически в результате работы программы. Строка оканчивается нуль-символом. Возвращаемое значение - указатель на подстроку.
char** string_parser(char* string, char* delim);	//Преобразует строку в вектор подстрок, разделенных символами из строки delim. Деление строки происходит с учетом кавычек. Вектор оканчивается NULL-указателем.
int controls_init();	//Копирование начальных настроек терминала

