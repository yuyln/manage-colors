all: main.c
	gcc -fsanitize=address -fsanitize=undefined -Wall -Wextra -pedantic -O0 -g3 -ggdb -o main main.c
