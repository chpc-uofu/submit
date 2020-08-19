###################################################################
#
#       Make master-slave work submit code with master also working (via OpenMP)
#
###################################################################
SHELL=/bin/bash

CFLAGS  = -g -qopenmp 
LFLAGS  = -qopenmp  
LIB     = -c 
MPI     =
#
#
# Use Intel for now
FC      = mpiifort
CC      = mpiicc
OBJ     = 

OBJS    = submit.o work.o ctimer.o

all: submit test

submit:$(OBJS)	
	$(CC) -o submit $(OBJS) $(LFLAGS) 
	@if [ ! -d "bin" ]; then mkdir bin; fi;
	mv submit bin
	make -C tests

test: 
	cd tests; mpirun -genv I_MPI_WAIT_MODE 1 -np 2 ../bin/submit

clean: 
	rm -f submit *.o 
	make clean -C tests
#
#
#

.SUFFIXES: .f .c .o 

.c.o:
	$(CC) $(LIB) $(CFLAGS) $<

.f.o:
	$(FC) $(LIB) $(CFLAGS) $<
