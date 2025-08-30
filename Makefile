main: main.c game.o
	gcc main.c -o main game.o -O3 -lwinmm -lgdi32

game.o: game.c
	gcc game.c -o game.o -c