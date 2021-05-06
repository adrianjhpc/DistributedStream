# DistributedStream
A hybrid distributed stream benchmark, designed to run STREAMs like benchmarking across a range of compute nodes and varying MPI and  OpenMP configurations. It is designed to allow running of different memory tasks, and currently supports evaluating main memory performance and persistent memory performance using PMDK functionality. 

The benchmark will run across multiple nodes and identify performance variation between nodes. As such it may be useful to identifying nodes with poor memory performance and characterising memory bandwidth variation over systems.

It also has persistent memory support through the PMDK library, with a number of different persistent memory configurations.

## Dependencies
This program depends on the [https://github.com/michaelrsweet/mxml](mxml) library for outputing results. The current Makefile assumes mxml is installed in your home directory, if this is not the case change these lines in the Makefile to point to where mxml headers and libraries are installed:

```
MXMLINC=-I${HOME}/mxml/include
MXMLLIB=-L${HOME}/mxml/lib -lmxml
``` 

If you want to build with persistent memory functionality then the [https://github.com/pmem/pmdk/](PMDK)  and [https://github.com/memkind/memkind](memkind) libraries should also be installed. Depending upon how you install these libraries you may have to alter the Makefile for a successful build.

## Building
By default the non-persistent memory version is built, producing a single exectuable (`distributed_streams`) when the `make` command is run. You will need to add a compiler to the makefile by altering this line in the Makefile to choose an appropriate compiler for you system:

```
CC      = 
```

Should you wish to build the persistent memory version of the benchmark you can use the command `make distributed_streams_pmem` which will produce an executable named `distributed_streams_pmem`, or `make distributed_streams_memkind` which will produce an exectuable named `distributed_streams_memkind`. These require the PMDK library `lpmem` and the Memkind library `lmemkind` respectively. The library and header paths for these libraries can be added to the Makefile if required.

For the PMDK benchmarks it is possible to build in two different ways. The first method, which will be enabled by default, assumes there are multiple persistent memory mount points, one per socket, and automatically chooses the closest mount point for each process. The second method, enabled by adding in the `-DPMEM_STRIPED` parameter to the `CFLAGSPMEM` line in the Makefile, assumes that there is a single persistent memory mount point that has been manually striped across all available persistent memory.

## Running
To run the benchmark specify the number if MPI processes and OpenMP threads as you would for an other MPI/OpenMP program (you can run without using OpenMP threads by setting the number of threads to 1). The application requires that you provide the following things on the command line when running it:

* Size of the last level of cache: Integer which specifies the number of elements to be used for each array created. Because we want each array to be four times the size of the last level of cache, the total memory used per process will be 4 x (last level of cache as specified by the user) x 3 (the number of arrays used in the benchmark).
* Number of repeats: Integer specifying how many times to run each benchmark
* Persistent memory path: String specifying the persistent memory location (this is optional, and only required by the `distributed_streams_pmem` and `distributed_streams_memkind` executables).

