# DistributedStream
A hybrid distributed stream benchmark, designed to run STREAMs like benchmarking across a range of compute nodes and varying MPI and OpenMP configurations. It is designed to allow running of different memory tasks, and currently supports evaluating main memory performance and persistent memory performance using PMDK functionality. 

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

## Interpreting results
When run the program will print out results of the following form:
```
Stream Memory Task
This system uses 8 bytes per array element.
Array size = 5000000 (elements), Offset = 0 (elements)
Memory per array = 38.1 MiB (= 0.0 GiB).
Total memory required per process = 114.4 MiB (= 0.1 GiB).
Total memory required per node = 5493.2 MiB (= 5.4 GiB).
Each kernel will be executed 10 times.
 The *best* time for each kernel (excluding the first iteration)
 will be used to compute the reported bandwidth.
Running with 1104 MPI processes, each with 1 OpenMP threads. 48 processes per node
Benchmark   Average Bandwidth    Avg Time    Max Bandwidth   Min Time    Min Bandwidth   Max Time   Max Time Location
                  (MB/s)         (seconds)       (MB/s)      (seconds)       (MB/s)      (seconds)      (proc name)
----------------------------------------------------------------------------------------------------------------------
Copy:           2826.8:      0.028300:        2902.9:      0.027559:         2741.1:      0.029185   cn04
Scale:          2803.4:      0.028537:        2885.8:      0.027722:         2704.0:      0.029586   cn12
Add:            3206.7:      0.037422:        3263.3:      0.036773:         3115.3:      0.038520   cn11
Triad:          3213.7:      0.037340:        3277.9:      0.036609:         2997.4:      0.040035   cn17
Node Copy:      135686.9:      0.028300:      134939.7:      0.028457:       131574.2:      0.029185
Node Scale:     134561.0:      0.028537:      133905.3:      0.028677:       129790.8:      0.029586
Node Add:       153921.6:      0.037422:      152663.7:      0.037730:       149532.3:      0.038520
Node Triad:     154258.6:      0.037340:      152911.1:      0.037669:       143874.1:      0.040035
```

The Copy, Scale, Add, and Triad results are equivalent to what is provided by the standard STREAMs benchmark. The "Node" versions of those results (i.e. Node Copy, Node Scale, etc...) present results by aggregating data from processes running on individual nodes. When aggregating data from nodes the minimum and maximum results are collected in a different manner to the single process results, and this can lead to the average performance being higher than the maximum, as they are calculated in different ways. The average for the nodes is simply the sum of all the process results for a node across all repeats of the benchmark, divided by the total number of times the benchmark is run. However, the minimum and maximum values are collected for individual runs of the benchmark. Therefore, if we are running the benchmark 10 times as in the above example (`Each kernel will be executed 10 times.`), the we collect the per node value for each run of the benchmark, and calculate the minimum and maximum from that data. This is to ensuring that we are really measuring the node memory bandwidth when processes are running concurrently, rather than mixing data from different runs which could produce maximum values that are unachievable in real world usage.

As well as printing out the statistics shown above, the benchmark also creates a file (i.e. `memory_results-PxT-timestamp.dat`, where the `P` represents the number of processes per node used, and the `T` represents the number of threads used, and `timestamp` is when the benchmark ran) with all the individual node results. We include a python program (`process_results.py`) to create a heat map of these individual node results from this file, which can be run as follows (replacing the filename at the end with the specific data file you want to visualise):

```
python prcoess_results.py memory_results-48x1-100101042021.dat
```

