all:
	gcc tterm.c -o tterm
	
sanitized: 
	gcc tterm.c -o tterm -fsanitize=address
