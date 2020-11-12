#include "tterm.h"
#define _GNU_SOURCE
int debug_mode=1;


int main(int argc, char *argv[])
{
	char string[MAX_LENGTH];
	char path[MAX_PATH_LENGTH];
	char* command;
	char **arg_vec;
	int arg_count;
	char tmp;
	int i;
	char is_word;
	int is_in_bracket=0;
	getcwd(path,MAX_PATH_LENGTH);
	while(1)
	{
		//Начало
		printf("%s: >",path);
		//Ввод строки
		fgets(string, MAX_LENGTH, stdin);
		arg_vec = string_parser(string, " \n");
		command=arg_vec[0];
		arg_vec[0]=path;
		//Проверка на встроенные команды
		if (strcmp(command, "cd")==0)
		{
			shell_cd(path, arg_vec);
			continue;
		}
		//Раздвоение
		int child=fork();
		int child_state;
		switch (child)
		{
			case 0:
				if (execvp(command, arg_vec)==-1)
				{

				}
				return 1;
			case -1:
				printf("ERROR 404 - бляздец\n");
				break;
			default:
				signal(SIGINT, kill_child);	//Перехватываем сигнал, идущий в предка
				waitpid(child, &child_state, 0);
				signal(SIGINT, SIG_DFL);	//Возвращаем стандартное поведение при сигнале прерывания
				break;
		}
    }
	return 0;
}

void kill_child(int param)
{
	if (debug_mode==1)
		printf("\nClosed\n");
	else
		printf("\n");	
}
