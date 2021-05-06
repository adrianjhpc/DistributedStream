SRCMPI	= streams_memory_task.c main_program.c utilities.c

OBJMPI	=$(SRCMPI:.c=.o)

SRCPMEM  = streams_memkind_memory_task.c streams_persistent_memory_task.c streams_read_persistent_memory_task.c streams_write_persistent_memory_task.c streams_memory_task.c main_program.c utilities.c

OBJPMEM  =$(SRCPMEM:.c=.pmem)

CC      = cc
PP     = -DLAST_LEVEL_CACHE_SIZE=30000000 -DNTIMES=10

MXMLINC=-I/home/z19/z19/adrianj/mxml/3.1/include
MXMLLIB=-L/home/z19/z19/adrianj/mxml/3.1/lib -lmxml

LDFLAGS = -fopenmp
CFLAGS = $(LDFLAGS) -g  -O3 -ffreestanding -fopenmp $(MXMLINC)  $(PP)
CFLAGSPMEM = $(CFLAGS) -DPMEM
LIBS    =$(MXMLLIB)

PRGMPI	= distributed_streams
PRGPMEM = distributed_streams_pmem

main:	$(PRGMPI) 

%.o:%.c	Makefile
	$(CC) -c $(CFLAGS) $<

%.pmem: %.c Makefile
	$(CC) -c -o $@ $(CFLAGSPMEM) $<

$(PRGMPI):$(OBJMPI) Makefile definitions.h
	$(CC) $(LDFLAGS) -o $@ $(OBJMPI) $(LIBS)
	rm -fr *.o

$(PRGPMEM):$(OBJPMEM) Makefile definitions.h
	$(CC) $(LDFLAGS) -o $@ $(OBJPMEM) $(LIBS)
	rm -fr *.pmem


clean:
	rm -fr $(TMP) $(OBJMPI) $(PRGMPI) $(OBJPMEM) $(PRGPMEM) core
