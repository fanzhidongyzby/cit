EXE=ass
CC=g++
OBJ=main.o lexial.o parser.o semantic.o generator.o elf_file.o
CFLAGS=-g -Wno-deprecated
$(EXE):$(OBJ)
	$(CC) $(OBJ) -o $(EXE) $(CFLAGS)
	rm -f $(OBJ) *~
clean:
	rm -f $(EXE) $(OBJ) *~
