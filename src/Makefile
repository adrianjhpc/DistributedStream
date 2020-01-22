SRCMPI	= streams_persistent_memory_task.c streams_write_persistent_memory_task.c streams_read_persistent_memory_task.c streams_memkind_memory_task.c streams_memory_task.c main_program.c 

OBJMPI	=$(SRCMPI:.c=.o)

CC	= mpiicc
#PP     = -DLAST_LEVEL_CACHE_SIZE=8600000000 -DNTIMES=5
PP	= -DLAST_LEVEL_CACHE_SIZE=30000000 -DNTIMES=10#-DLAST_LEVEL_CACHE_SIZE=11100000000 -DNTIMES=5#-DLAST_LEVEL_CACHE_SIZE=30000000 -DNTIMES=10 #-DLAST_LEVEL_CACHE_SIZE=10000000 -DNTIMES=20  

LDFLAGS = -fopenmp
CFLAGS = -fopenmp -O3 -ffreestanding -I/home/software/memkind/1.9.0/include  $(PP)
#CFLAGS  = -fopenmp -O3 -ffreestanding -qopt-streaming-stores always -I/home/software/memkind/1.9.0/include  $(PP)
LIBS    = -L/home/nx01/nx01/adrianj/pmdk/1.6/lib -lpmem -L/home/software/memkind/1.9.0/lib -lmemkind 

PRGMPI	= distributed_streams

main:	$(PRGMPI) 

.c.o:	Makefile
	$(CC) -c $(CFLAGS) $(ELIBS) $<

$(PRGMPI):$(OBJMPI) Makefile definitions.h
	$(CC) $(LDFLAGS) -o $@ $(OBJMPI) $(LIBS)

clean:
	rm -fr $(TMP) $(OBJMPI) $(PRGMPI) core
