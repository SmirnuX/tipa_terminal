#include "tterm.h"

int debug_mode=1;


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
	controls_init();
	
	while(1)
	{
		tcsetattr(input, TCSANOW, &default_settings);
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
		int pipe_desc[2];	//0 - чтение, 1 - запись
		pipe(pipe_desc);
		tcsetattr(input, TCSANOW, &new_settings);
		switch (child)
		{
			case 0:
				close(0);	//Закрываем ввод с клавиатуры
				dup(pipe_desc[0]);	//Стандартный ввод заменяется каналом 
				close(pipe_desc[0]);
				close(pipe_desc[1]);
				execvp(command, arg_vec);
				perror("");
				return 1;
			case -1:
				printf("ERROR 404 - бляздец\n");
				break;
			default:
				close(pipe_desc[0]);
				char s = 12;
				write(pipe_desc[1], &s, 1);
				close(pipe_desc[1]);
				while(waitpid(child, &child_state, WNOHANG)!=child)	//Проверяем, закончилось ли исполнение дочернего процесса
				{
					s = fgetc(stdin);
					if (s==0x3)
					{
						kill(child, SIGINT);
						break;
					}
					if (s!=EOF)
					{
						printf("%c", s);
						write(pipe_desc[1], &s, 1);
					}
				}  	
				close(pipe_desc[1]);
				break;
		}
		tcsetattr(input, TCSANOW, &default_settings);
    }
	return 0;
}

int control()
{
	return getc(stdin);
}
