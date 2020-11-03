objects = tterm.c string_parser.c
output = tterm

all:
	gcc $(objects) -o $(output)
	
sanitized: 
	gcc $(objects) -o $(output) -fsanitize=address
