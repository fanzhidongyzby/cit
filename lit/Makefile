EXE=lit
CC=g++
OBJ=main.o elf_file.o linker.o
CFLAGS=-g -Wno-deprecated
$(EXE):$(OBJ)
	$(CC) $(OBJ) -o $(EXE) $(CFLAGS)
	rm $(OBJ) *~ -f
clean:
	rm $(EXE) $(OBJ) *~ -f
