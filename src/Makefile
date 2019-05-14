SRCMPI	= streams_write_persistent_memory_task.c streams_read_persistent_memory_task.c streams_persistent_memory_task.c streams_memory_task.c main_program.c 

OBJMPI	=$(SRCMPI:.c=.o)

CC	= mpiicc
PP	= -g -DLAST_LEVEL_CACHE_SIZE=10000000 -DNTIMES=5 #-DLAST_LEVEL_CACHE_SIZE=10000000 -DNTIMES=20  

LDFLAGS = -fopenmp
CFLAGS  = -fopenmp -ffreestanding -O3 $(PP)
LIBS    = -L/home/nx01/nx01/adrianj/pmdk/1.6/lib -lpmem 

PRGMPI	= distributed_streams

main:	$(PRGMPI) 

.c.o:	Makefile
	$(CC) -c $(CFLAGS) $(ELIBS) $<

$(PRGMPI):$(OBJMPI) Makefile definitions.h
	$(CC) $(LDFLAGS) -o $@ $(OBJMPI) $(LIBS)

clean:
	rm -fr $(TMP) $(OBJMPI) $(PRGMPI) core
