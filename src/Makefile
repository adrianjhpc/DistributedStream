SRCMPI	= streams_memory_task.c main_program.c utilities.c
OBJMPI	=$(SRCMPI:.c=.o)

SRCPMEM  = streams_persistent_memory_task.c streams_read_persistent_memory_task.c streams_write_persistent_memory_task.c streams_memory_task.c main_program.c utilities.c
OBJPMEM  =$(SRCPMEM:.c=.pmem)

SRCMEMKIND  = streams_memkind_memory_task.c streams_memory_task.c main_program.c utilities.c
OBJMEMKIND  =$(SRCMEMKIND:.c=.memkind)

CC     = 

MXMLINC=-I${HOME}/mxml/include
MXMLLIB=-L${HOME}/mxml/lib -lmxml

LIBS    =$(MXMLLIB)

LDFLAGS = -fopenmp
CFLAGS = $(LDFLAGS) -g  -O3 -ffreestanding -fopenmp $(MXMLINC)  $(PP)

CFLAGSPMEM = $(CFLAGS) -DPMEM #-DPMEM_STRIPED
LIBSPMEM = $(LIBS) -lpmem

CFLAGSMEMKIND = $(CFLAGS) -DMEMKIND
LIBSMEMKIND = $(LIBS) -lmemkind

PRGMPI	= distributed_streams
PRGPMEM = distributed_streams_pmem
PRGMEMKIND = distributed_streams_memkind

main:	$(PRGMPI) 

%.o:%.c	Makefile
	$(CC) -c $(CFLAGS) $<

%.pmem: %.c Makefile
	$(CC) -c -o $@ $(CFLAGSPMEM) $<

%.memkind: %.c Makefile
	$(CC) -c -o $@ $(CFLAGSMEMKIND) $<

$(PRGMPI):$(OBJMPI) Makefile definitions.h
	$(CC) $(LDFLAGS) -o $@ $(OBJMPI) $(LIBS)
	rm -fr *.o

$(PRGPMEM):$(OBJPMEM) Makefile definitions.h
	$(CC) $(LDFLAGS) -o $@ $(OBJPMEM) $(LIBSPMEM)
	rm -fr *.pmem

$(PRGMEMKIND):$(OBJMEMKIND) Makefile definitions.h
	$(CC) $(LDFLAGS) -o $@ $(OBJMEMKIND) $(LIBSMEMKIND)
	rm -fr *.memkind

clean:
	rm -fr $(TMP) $(OBJMPI) $(PRGMPI) $(OBJPMEM) $(PRGPMEM) $(OBJMEMKIND) $(PRGMEMKIND) core
