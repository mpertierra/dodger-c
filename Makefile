all: dodger

dodger: dodger.c
	gcc dodger.c -lSDL2_mixer -lSDL2_image -lSDL2_ttf -lSDL2 -o dodger
