main: main.c build/game.o build/glad.o
	gcc main.c -o build/main build/glad.o build/game.o -lwinmm -lgdi32 -lopengl32

build/glad.o: deps/glad/glad.c
	gcc deps/glad/glad.c -o build/glad.o -c

build/game.o: game.c
	gcc game.c -o build/game.o -c