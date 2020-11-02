#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_LENGTH 512
#define MAX_ARGS 20

int main(int argc, char *argv[])
{
	char string[MAX_LENGTH];
	char command[MAX_LENGTH];
	char **arg_vec;
	int arg_count;
	char tmp;
	int i;
	char is_word;
	
	//Ввод строки
	fgets(string, MAX_LENGTH, stdin);
	//Подсчитывание количества аргументов
	arg_count = 0;
	is_word = 0;
	for(i=0; i<MAX_LENGTH-1; i++)
	{
		if (string[i]=='\n' || string[i]=='\0')
			break;
		if(string[i]!=' ')
		{
			if (is_word==0)
			{
				is_word=1;
				arg_count++;
			}
		}
		else
			is_word=0;
	}
	//Выделение памяти под вектор
	arg_vec = malloc(arg_count);
	for (i=0; i<arg_count; i++)
	{
		if (i==arg_count)
			arg_vec[i]=NULL;
		else
		{
			strtok(string," \t");
			arg_vec[i]=string;
		}
	}
	
	for (i=0; i<arg_count-1; i++)
	{
		printf("%s, ", arg_vec[i]);
	}
	
	//Разбиваем на лексемы
	printf("%i", arg_count);
  execvp(string, arg_vec);
	return 0;
}
