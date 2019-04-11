SRCMPI	= streams_memory_task.c main_program.c 

OBJMPI	=$(SRCMPI:.c=.o)

CC	= mpiicc
PP	= -g -DSTREAM_ARRAY_SIZE=80000000 -DNTIMES=20  

LDFLAGS = -fopenmp
CFLAGS  = -fopenmp -ffreestanding -O3 $(PP)
LIBS    = 

PRGMPI	= distributed_streams

main:	$(PRGMPI) 

.c.o:	Makefile
	$(CC) -c $(CFLAGS) $(ELIBS) $<

$(PRGMPI):$(OBJMPI) Makefile definitions.h
	$(CC) $(LDFLAGS) -o $@ $(OBJMPI) $(LIBS)

clean:
	rm -fr $(TMP) $(OBJMPI) $(PRGMPI) core
