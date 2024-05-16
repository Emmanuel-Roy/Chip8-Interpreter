all:
	g++ -g -I src/include -L src/lib -o Chip8-Emulator chip8.cpp -lmingw32 -lSDL2main -lSDL2