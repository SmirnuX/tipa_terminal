// SPDX-License-Identifier: CPOL-1.02
#include "tterm.h"

char *new_str_copy(char *source, int beginning, int end)
{
	char *substr = malloc(end - beginning + 1);

	for (int i = beginning; i < end; i++)
		substr[i-beginning] = source[i];
	substr[end - beginning] = '\0';	//"Закрытие" строки
	return substr;
}

char **string_parser(char *string, char *delim)
{
	int i;
	int begword = 0;	//Начало аргумента
	int in_brackets = 0;	//Находится ли i-ый символ внутри скобок
	int is_word = 0;	//Находится ли i-ый символ внутри строки
	int count = 0;	//Количество лексем в строке
	//Подсчитывание количества аргументов
	for (i = 0; string[i] != '\0'; i++)	{
		if (string[i] == '\"')	{
			if (is_word == 1)	{
				is_word = 0;
				count++;
			}
			if (in_brackets == 0 && string[i+1] != '\0' && string[i+1] != '\"' && string[i+1] != '\n')
				in_brackets = 1;
			else if (in_brackets == 1)	{
				in_brackets = 0;
				count++;
			}
		}	else if (in_brackets == 0)	{
			if (strchr(delim, string[i]) == NULL && is_word == 0)
				is_word = 1;
			else if (strchr(delim, string[i]) != NULL && is_word == 1)	{
				is_word = 0;
				count++;
			}
		}
	}
	if (is_word == 1 || in_brackets == 1)
		count++;
	char **arg_vec = malloc((count + 1) * sizeof(char *));
	int j = 0;

	in_brackets = 0;
	is_word = 0;
	//Заполнение вектора
	for (i = 0; string[i] != '\0'; i++)	{
		if (string[i] == '\"')	{
			if (is_word == 1)	{
				is_word = 0;
				arg_vec[j] = new_str_copy(string, begword, i);
				j++;
			}
			if (in_brackets == 0 && string[i+1] != '\0' && string[i+1] != '\"' && string[i+1] != '\n')	{
				in_brackets = 1;
				begword = i + 1;
			}	else if (in_brackets == 1)	{
				in_brackets = 0;
				arg_vec[j] = new_str_copy(string, begword, i);
				j++;
			}
		}	else if (in_brackets == 0)	{
			if (strchr(delim, string[i]) == NULL && is_word == 0)	{
				is_word = 1;
				begword = i;
			}	else if (strchr(delim, string[i]) != NULL && is_word == 1)	{
				is_word = 0;
				arg_vec[j] = new_str_copy(string, begword, i);
				j++;
			}
		}
	}
	if (is_word == 1 || in_brackets == 1)	{
		arg_vec[j] = new_str_copy(string, begword, i);
		j++;
	}
	arg_vec[j] = NULL;
	//Вывод для отладки
	if (debug_mode == 1)	{
		printf("Всего лексем: %i:\n", count);
		for (i = 0; i < j; i++)
			printf("%i: %s\n", i+1, arg_vec[i]);
	}
	return arg_vec;
}
