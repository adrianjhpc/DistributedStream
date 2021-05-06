SRCMPI	= streams_memory_task.c main_program.c utilities.c

OBJMPI	=$(SRCMPI:.c=.o)

SRCPMEM  = streams_persistent_striped_memory_task.c streams_memkind_memory_task.c streams_persistent_memory_task.c streams_read_persistent_memory_task.c streams_write_persistent_memory_task.c streams_memory_task.c main_program.c utilities.c

OBJPMEM  =$(SRCPMEM:.c=.pmem)

CC      = mpiicc
PP     = -DLAST_LEVEL_CACHE_SIZE=30000000 -DNTIMES=10

MXMLINC=-I${HOME}/mxml/include
MXMLLIB=-L${HOME}/mxml/lib -lmxml

LDFLAGS = -fopenmp
CFLAGS = $(LDFLAGS) -g  -O3 -ffreestanding -fopenmp $(MXMLINC)  $(PP)
CFLAGSPMEM = $(CFLAGS) -DPMEM
LIBS    =$(MXMLLIB)

FLAGSPMEM = $(CFLAGS) -DPMEM
LIBSPMEM = $(LIBS) -lpmem -lmemkind

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
	$(CC) $(LDFLAGS) -o $@ $(OBJPMEM) $(LIBSPMEM)
	rm -fr *.pmem


clean:
	rm -fr $(TMP) $(OBJMPI) $(PRGMPI) $(OBJPMEM) $(PRGPMEM) core
