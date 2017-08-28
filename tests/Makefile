###################################################################
#
#       Make master-slave work submit code with master also working (via OpenMP)
#
###################################################################
CFLAGS  = 
LFLAGS  = 
LIB     = -c 
MPI     =  
#
#
# Must use pgcc since gcc does not have OpenMP support
FC      = 
CC      = gcc
OBJ     = 
#
#.f.o:
#	$(F90) -c  $*.f
#.c.o:
#	$(CC) -c   $*.c
#

OBJS    = test.o

.SUFFIXES: .f .c .o 

.c.o:
	$(CC) $(LIB) $(CFLAGS) $<

.f.o:
	$(FC) $(LIB) $(CFLAGS) $<


test:$(OBJS)	
	$(CC) -o test $(OBJS) $(LFLAGS) 


clean:
	rm -f test *.o *.out
#
#
