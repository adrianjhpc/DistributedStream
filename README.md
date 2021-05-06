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
By default the non-persistent memory version is built, producing a single exectuable (`distributed_streams`) when the `make` command is run. You may need to alter this line in the Makefile to choose an appropriate compiler for you system:

```
CC      = cc
```

Should you wish to build the persistent memory version of the benchmark you can use the command `make distributed_streams_pmem` which will produce an exectuablec named `distributed_streams_pmem`. 
