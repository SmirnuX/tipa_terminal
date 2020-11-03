#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LENGTH 512
#define MAX_ARGS 20

int debug_mode=1;


/*
Необходимые фичи:
 - Выполнение команд +
 - Раздвоение +
 - Обработка строк
 - Управление - перемещение влево/вправо по строке, предыдущие/следующие команды
 - "Трубы" и вывод в файлы
 - Обработка ошибок
*/

int main(int argc, char *argv[])
{
	char string[MAX_LENGTH];
	char* command;
	char **arg_vec;
	int arg_count;
	char tmp;
	int i;
	char is_word;
	while(1)
	{
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
	arg_vec = malloc(arg_count+1);	//Выделение памяти под вектор
	arg_vec[0]=argv[0];	//Передаем путь
	char* tmpstring=strtok(string, " \t\n");
	if (tmpstring==NULL)
	{
	//TODO - рассмотреть пустую строку
	}
	else
	{
		command=malloc(strlen(tmpstring)+1);
		strcpy(command, tmpstring);
		
		if(debug_mode==1)
		{
			printf("Команда: %s\n", command);
			printf("%s", getenv("PATH"));
			printf("Всего аргументов: %i:\n", arg_count-1);
		}	
		for (i=1; i<=arg_count; i++)
		{
			if (i==arg_count)
			{
				arg_vec[i]=NULL;
				if(debug_mode==1)
					printf("%i - NULL.\n===============\n", i);
			}
			else
			{
				tmpstring = strtok(NULL," \t\n");
				if(debug_mode==1)
					printf("%i - %s,\n", i, tmpstring);
				arg_vec[i]=malloc(strlen(tmpstring)+1);	//Выделяем память под аргументы
				strcpy(arg_vec[i], tmpstring);
			}
		}
		//Раздвоение
		int child=fork();
		int child_state;
		switch (child)
		{
			case 0:
				execvp(command, arg_vec);
				perror("");
				break;
			case -1:
				printf("ERROR 404 - бляздец\n");
			default:
				waitpid(1, &child_state, 0);  	//Ждем пока сына в институт поступит и из хаты свалит наконец
				break;
		}
    }
    }
	return 0;
}
