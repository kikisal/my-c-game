main: main.c game.o glad.o
	gcc main.c -o main glad.o game.o -O3 -lwinmm -lgdi32 -lopengl32

glad.o: deps/glad/glad.c
	gcc deps/glad/glad.c -o glad.o -c

game.o: game.c
	gcc game.c -o game.o -c