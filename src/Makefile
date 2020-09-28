SRCTEST = test_hostname_hashing.c utilities.c
SRCMPI	= streams_memory_task.c utilities.c main_program.c 

OBJMPI	=$(SRCMPI:.c=.o)
OBJTEST =$(SRCTEST:.c=.o)

CC	= mpicc
#PP     = -DLAST_LEVEL_CACHE_SIZE=8600000000 -DNTIMES=5
PP	= -DLAST_LEVEL_CACHE_SIZE=200000000 -DNTIMES=10#-DLAST_LEVEL_CACHE_SIZE=11100000000 -DNTIMES=5#-DLAST_LEVEL_CACHE_SIZE=30000000 -DNTIMES=10 #-DLAST_LEVEL_CACHE_SIZE=10000000 -DNTIMES=20  

MXMLINC=-I/lustre/home/z04/adrianj/mxml/include
MXMLLIB=-L/lustre/home/z04/adrianj/mxml -lmxml

LDFLAGS = -fopenmp
#CFLAGS = -g -fopenmp -ffreestanding $(MXMLINC)  $(PP)
CFLAGS  = -fopenmp -O3 -ffreestanding -qopt-streaming-stores always $(MXMLINC)  $(PP)
LIBS    =  $(MXMLLIB) 

PRGMPI	= distributed_streams
PRGTEST = test_hostname_hashing

main:	$(PRGMPI) 

.c.o:	Makefile
	$(CC) -c $(CFLAGS) $(ELIBS) $<

$(PRGMPI):$(OBJMPI) Makefile definitions.h
	$(CC) $(LDFLAGS) -o $@ $(OBJMPI) $(LIBS)

$(PRGTEST):$(OBJTEST) Makefile definitions.h
	$(CC) $(LDFLAGS) -o $@ $(OBJTEST) 

clean:
	rm -fr $(TMP) $(OBJMPI) $(OBJTEST) $(PRGMPI) $(PRGTEST) core
