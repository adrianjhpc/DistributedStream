# DistributedStream
A hybrid distributed stream benchmark, designed to run STREAMs like benchmarking across a range of compute nodes and varying MPI and  OpenMP configurations. It is designed to allow running of different memory tasks, and currently supports evaluating main memory performance and persistent memory performance using PMDK functionality. 

The benchmark will run across multiple nodes and identify performance variation between nodes. As such it may be useful to identifying nodes with poor memory performance and characterising memory bandwidth variation over systems.

It also has persistent memory support through the PMDK library, with a number of different persistent memory configurations.
