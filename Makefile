main: main.c
	gcc main.c -o main -lwinmm -lgdi32 -lpthread -O3