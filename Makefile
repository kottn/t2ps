
.SUFFIXES : 
.SUFFIXES : .o .x .f .F .c

.x.o : 
	xy77 -c -G -v $*.x

.f.o :
	f77 -c -g $<

.F.o :
	xy77 -c -g $<

.c.o :
	$(CC) -c -g $<

EXE = ${HOME}/local/bin/t2ps
OBJ = t2ps.o page.o line.o 
HDR = t2ps.h

all : $(EXE)

clean :
	rm -f $(EXE) $(OBJ) *~ *core

$(EXE) : $(OBJ)
	$(CC) -o $(EXE) $(OBJ) -lm

$(OBJ) : $(HDR)


