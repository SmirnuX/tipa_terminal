#include "tterm.h"


int main(int argc, char *argv[])
{
	char string[MAX_LENGTH];
	char* command;
	char **arg_vec;
	int arg_count;
	char tmp;
	int i;
	char is_word;
	int is_in_bracket=0;
	while(1)
	{
		//Начало
		printf("%s: >",getenv("PWD"));
		//Ввод строки
		fgets(string, MAX_LENGTH, stdin);
		
		arg_vec = string_parser(string, " \n");
		command=arg_vec[0];
		arg_vec[0]=getenv("PWD");
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
				waitpid(child, &child_state, 0);  	//Ждем пока сына в институт поступит и из хаты свалит наконец
				break;
		}
    }
	return 0;
}
