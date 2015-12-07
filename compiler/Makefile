EXE=compiler
CC=g++
OBJ=main.o compiler.o lexial.o parser.o error.o semantic.o generator.o
CFLAGS=-g -Wno-deprecated
$(EXE):$(OBJ)
	$(CC) $(OBJ) -o $(EXE) $(CFLAGS)
	rm $(OBJ) *~ -f
clean:
	rm $(EXE) $(OBJ) *~ -f
