#include "tterm.h"

//		Управление настройками терминала, в целях управления с помощью клавиатуры

struct termios default_settings, new_settings;
int input, output;

int controls_init()
{
	int input, output;
	input=open("/dev/tty", O_RDONLY);
	output=open("/dev/tty", O_WRONLY);
	tcgetattr(input, &default_settings);	//Получаем изначальные настройки терминала
	default_settings.c_lflag &= ~ISIG;
	new_settings=default_settings;
	new_settings.c_lflag &= ~ICANON;	//Переводим в неканонический режим
	new_settings.c_lflag &= ~ECHO;	//Убираем вывод каждого символа
	new_settings.c_cc[VMIN] = 0;	//Минимальный объем ввода - один символ
	new_settings.c_cc[VTIME] = 0;	//Минимальный промежуток между вводами - отсутствует
	new_settings.c_lflag &= ~ISIG;	//Создание сигналов
}

