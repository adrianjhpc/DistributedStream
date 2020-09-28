#include "definitions.h"
#include "utilities.h"

int main(int argc, char **argv){

  int temp_size, temp_rank;
  MPI_Comm temp_comm;
  int node_key;
  int array_size;
  int socket, core;
  int performing_persistent = 0;
  int performing_memkind = 0;
  benchmark_results b_results;
  aggregate_results node_results;
  benchmark_results *all_node_results;
  aggregate_results a_results;
  communicator world_comm, node_comm, root_comm;
  char *filename;

  filename = (char *)malloc(sizeof(char)*MAX_FILE_NAME_LENGTH);

#ifdef PMEM
  if(argc != 3){
    printf("Expecting two parameters (each 0 or 1) specifying whether memkind memory and/or persistent memory tasks should be run\n");
    exit(0);
  }else{
    performing_memkind = atoi(argv[1]);
    if(performing_memkind != 0 && performing_memkind != 1){
      printf("Expecting a parameter (0 or 1) specifying whether memkind memory tasks should be run. Current parameter is not 0 or 1.\n");
      exit(0);
    }
    performing_persistent = atoi(argv[2]);
    if(performing_persistent != 0 && performing_persistent != 1){
      printf("Expecting a parameter (0 or 1) specifying whether persistent memory tasks should be run. Current parameter is not 0 or 1.\n");
      exit(0);
    }
  }
#else
  performing_persistent = 0;
  performing_memkind = 0;
#endif

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &temp_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &temp_rank);

  world_comm.comm = MPI_COMM_WORLD;
  world_comm.rank = temp_rank;
  world_comm.size = temp_size;

  // Get a integer key for this process that is different for every node
  // a process is run on.
  node_key = get_key();

  // Use the node key to split the MPI_COMM_WORLD communicator
  // to produce a communicator per node, containing all the processes
  // running on a given node.
  MPI_Comm_split(world_comm.comm, node_key, 0, &temp_comm);

  // Get the rank and size of the node communicator this process is involved
  // in.
  MPI_Comm_size(temp_comm, &temp_size);
  MPI_Comm_rank(temp_comm, &temp_rank);

  node_comm.comm = temp_comm;
  node_comm.rank = temp_rank;
  node_comm.size = temp_size;

  // Now create a communicator that goes across nodes. The functionality below will
  // create a communicator per rank on a node (i.e. one containing all the rank 0 processes
  // in the node communicators, one containing all the rank 1 processes in the
  // node communicators, etc...), although we are really only doing this to enable
  // all the rank 0 processes in the node communicators to undertake collective operations.
  MPI_Comm_split(world_comm.comm, node_comm.rank, 0, &temp_comm);

  MPI_Comm_size(temp_comm, &temp_size);
  MPI_Comm_rank(temp_comm, &temp_rank);

  root_comm.comm = temp_comm;
  root_comm.rank = temp_rank;
  root_comm.size = temp_size;
	
  all_node_results = malloc(root_comm.size * sizeof(struct benchmark_results));

  get_processor_and_core(&socket, &core);

  initialise_benchmark_results(&b_results);

  stream_memory_task(&b_results, world_comm, node_comm, &array_size);
  printf("Done streams_memory_task\n");
  collect_results(b_results, &a_results, &node_results, all_node_results, world_comm, node_comm, root_comm);
  printf("Done collect_results\n");
  if(world_comm.rank == ROOT){
    print_results(a_results, node_results, world_comm, array_size, node_comm);
    strcpy(filename, "memory_results.dat");
    save_results(filename, all_node_results, array_size, world_comm, node_comm, root_comm);
  }

  free_benchmark_results(&b_results);


  if(performing_memkind == 1){
#ifdef PMEM
    initialise_benchmark_results(&b_results);

    // Barrier here to ensure all processes are active and ready to start benchmarking
    // For performance results we only really need a per node barrier to ensure all
    // in a given node are at the same place, but this can avoid issues with multiple
    // processes removing or adding files (as in the persistent memory benchmarks) from
    // previous runs of the program.
    MPI_Barrier(world_comm.comm);
    stream_memkind_memory_task(&b_results, world_comm, node_comm, &array_size, socket);
    collect_results(b_results, &a_results, &node_results, all_node_results, world_comm, node_comm, root_comm);

    if(world_comm.rank == ROOT){
      print_results(a_results, node_results, world_comm, array_size, node_comm);
      strcpy(filename, "memkind_results.dat");
      save_results(filename, all_node_results, array_size, world_comm, node_comm, root_comm);
    }
 
    free_benchmark_results(&b_results);
#endif
  }

  if(performing_persistent == 1){
#ifdef PMEM
    initialise_benchmark_results(&b_results);

    // Barrier here to ensure all processes are active and ready to start benchmarking
    // For performance results we only really need a per node barrier to ensure all
    // in a given node are at the same place, but this can avoid issues with multiple
    // processes removing or adding files (as in the persistent memory benchmarks) from
    // previous runs of the program.
    MPI_Barrier(world_comm.comm);

    stream_persistent_memory_task(&b_results, world_comm, node_comm, &array_size, socket, none);
    collect_results(b_results, &a_results, &node_results, all_node_results, world_comm, node_comm, root_comm);

    if(world_comm.rank == ROOT){
      print_results(a_results, node_results, world_comm, array_size, node_comm);
      strcpy(filename, "persistent_memory_results.dat");
      save_results(filename, all_node_results, array_size, world_comm, node_comm, root_comm);
    }

    free_benchmark_results(&b_results);

 
    initialise_benchmark_results(&b_results);

    // Barrier here to ensure all processes are active and ready to start benchmarking
    // For performance results we only really need a per node barrier to ensure all
    // in a given node are at the same place, but this can avoid issues with multiple
    // processes removing or adding files (as in the persistent memory benchmarks) from
    // previous runs of the program.
    MPI_Barrier(world_comm.comm);

    stream_persistent_memory_task(&b_results, world_comm, node_comm, &array_size, socket, individual);
    collect_results(b_results, &a_results, &node_results, all_node_results, world_comm, node_comm, root_comm);

    if(world_comm.rank == ROOT){
      print_results(a_results, node_results, world_comm, array_size, node_comm);
      strcpy(filename, "individual_persistent_memory_results.dat");
      save_results(filename, all_node_results, array_size, world_comm, node_comm, root_comm);
    }

    free_benchmark_results(&b_results);

    initialise_benchmark_results(&b_results);

    // Barrier here to ensure all processes are active and ready to start benchmarking
    // For performance results we only really need a per node barrier to ensure all
    // in a given node are at the same place, but this can avoid issues with multiple
    // processes removing or adding files (as in the persistent memory benchmarks) from
    // previous runs of the program.
    MPI_Barrier(world_comm.comm);

    stream_persistent_memory_task(&b_results, world_comm, node_comm, &array_size, socket, collective);
    collect_results(b_results, &a_results, &node_results, all_node_results, world_comm, node_comm, root_comm);

    if(world_comm.rank == ROOT){
      print_results(a_results, node_results, world_comm, array_size, node_comm);
      strcpy(filename, "collective_persistent_memory_results.dat");
      save_results(filename, all_node_results, array_size, world_comm, node_comm, root_comm);
    }

    free_benchmark_results(&b_results);



    initialise_benchmark_results(&b_results);

    // Barrier here to ensure all processes are active and ready to start benchmarking
    // For performance results we only really need a per node barrier to ensure all
    // in a given node are at the same place, but this can avoid issues with multiple
    // processes removing or adding files (as in the persistent memory benchmarks) from
    // previous runs of the program.
    MPI_Barrier(world_comm.comm);

    stream_read_persistent_memory_task(&b_results, world_comm, node_comm, &array_size, socket);
    collect_results(b_results, &a_results, &node_results, all_node_results, world_comm, node_comm, root_comm);

    if(world_comm.rank == ROOT){
      print_results(a_results, node_results, world_comm, array_size, node_comm);
      strcpy(filename, "read_persistent_memory_results.dat");
      save_results(filename, all_node_results, array_size, world_comm, node_comm, root_comm);
    }

    free_benchmark_results(&b_results);


    initialise_benchmark_results(&b_results);

    // Barrier here to ensure all processes are active and ready to start benchmarking
    // For performance results we only really need a per node barrier to ensure all
    // in a given node are at the same place, but this can avoid issues with multiple
    // processes removing or adding files (as in the persistent memory benchmarks) from
    // previous runs of the program.
    MPI_Barrier(world_comm.comm);

    stream_write_persistent_memory_task(&b_results, world_comm, node_comm, &array_size, socket, none);
    collect_results(b_results, &a_results, &node_results, all_node_results, world_comm, node_comm, root_comm);

    if(world_comm.rank == ROOT){
      print_results(a_results, node_results, world_comm, array_size, node_comm);
      strcpy(filename, "write_persistent_memory_results.dat");
      save_results(filename, all_node_results, array_size, world_comm, node_comm, root_comm);
    }


    free_benchmark_results(&b_results);

 
    initialise_benchmark_results(&b_results);

    // Barrier here to ensure all processes are active and ready to start benchmarking
    // For performance results we only really need a per node barrier to ensure all
    // in a given node are at the same place, but this can avoid issues with multiple
    // processes removing or adding files (as in the persistent memory benchmarks) from
    // previous runs of the program.
    MPI_Barrier(world_comm.comm);

    stream_write_persistent_memory_task(&b_results, world_comm, node_comm, &array_size, socket, individual);
    collect_results(b_results, &a_results, &node_results, all_node_results, world_comm, node_comm, root_comm);

    if(world_comm.rank == ROOT){
      print_results(a_results, node_results, world_comm, array_size, node_comm);
      strcpy(filename, "individual_write_persistent_memory_results.dat");
      save_results(filename, all_node_results, array_size, world_comm, node_comm, root_comm);
    }

    free_benchmark_results(&b_results);

    initialise_benchmark_results(&b_results);

    // Barrier here to ensure all processes are active and ready to start benchmarking
    // For performance results we only really need a per node barrier to ensure all
    // in a given node are at the same place, but this can avoid issues with multiple
    // processes removing or adding files (as in the persistent memory benchmarks) from
    // previous runs of the program.
    MPI_Barrier(world_comm.comm);

    stream_write_persistent_memory_task(&b_results, world_comm, node_comm, &array_size, socket, collective);
    collect_results(b_results, &a_results, &node_results, all_node_results,  world_comm, node_comm, root_comm);

    if(world_comm.rank == ROOT){
      print_results(a_results, node_results, world_comm, array_size, node_comm);
      strcpy(filename, "collective_write_persistent_memory_results.dat");
      save_results(filename, all_node_results, array_size, world_comm, node_comm, root_comm);
    }

    free_benchmark_results(&b_results);
#endif
  }

  MPI_Finalize();

  free(all_node_results);
  free(filename);
}

void collect_results(benchmark_results b_results, aggregate_results *a_results, aggregate_results *node_results, benchmark_results *all_node_results, communicator world_comm, communicator node_comm, communicator root_comm){

  benchmark_type benchmark;

  benchmark = copy;
  collect_individual_result(b_results.Copy, &a_results->Copy, &node_results->Copy, a_results->copy_max, b_results.name, all_node_results, benchmark, world_comm, node_comm, root_comm);
  benchmark = scale;
  collect_individual_result(b_results.Scale, &a_results->Scale, &node_results->Scale, a_results->scale_max,  b_results.name, all_node_results, benchmark, world_comm, node_comm, root_comm);
  benchmark = add;
  collect_individual_result(b_results.Add, &a_results->Add, &node_results->Add, a_results->add_max, b_results.name, all_node_results, benchmark, world_comm, node_comm, root_comm);
  benchmark = triad;
  collect_individual_result(b_results.Triad, &a_results->Triad, &node_results->Triad, a_results->triad_max, b_results.name, all_node_results, benchmark, world_comm, node_comm, root_comm);

}

void collect_individual_result(performance_result indivi, performance_result *result, performance_result *node_result, char *max_name, char *name, benchmark_results *all_node_results, benchmark_type benchmark, communicator world_comm, communicator node_comm, communicator root_comm){

  // Structure to hold both a value and a rank for MAXLOC and MINLOC operations.
  // This *may* be problematic on some MPI implementations as it assume MPI_DOUBLE_INT
  // matches this specification.
  typedef struct resultloc {
    double value;
    int   rank;
  } resultloc;

  double temp_value;
  double temp_result;

  double temp_store, min_time_store, max_time_store;

  int k;

  double max_for_nodes[node_comm.size];
  double min_for_nodes[node_comm.size];
  double average_for_nodes[node_comm.size];
  char node_names[node_comm.size][MPI_MAX_PROCESSOR_NAME];

  // Variable for the result of the reduction
  resultloc rloc;
  // Variable for the data to be reduced
  resultloc iloc;

  int root = ROOT;
  MPI_Status status;

  MPI_Reduce(&indivi.avg, &result->avg, 1, MPI_DOUBLE, MPI_SUM, root, world_comm.comm);
  if(world_comm.rank == root){
    result->avg = result->avg/world_comm.size;
  }

  // Get the total avg value summed across all processes in a node to enable calculation
  // of the avg bandwidth for a node.
  temp_store = 0;
  for(k=1; k<NTIMES; k++) {
    temp_value = indivi.raw_result[k];
    MPI_Reduce(&temp_value, &temp_result, 1, MPI_DOUBLE, MPI_SUM, root, node_comm.comm);
    temp_result = temp_result/node_comm.size;
    temp_store = temp_store + temp_result;
  }
  temp_store = temp_store/(NTIMES-1);
  node_result->avg = temp_store;

  printf("aa\n");

  if(node_comm.rank == root){
    temp_value = node_result->avg;
    MPI_Reduce(&temp_value, &temp_result, 1, MPI_DOUBLE, MPI_SUM, root, root_comm.comm);
    if(world_comm.rank == root){
      node_result->avg = temp_result/root_comm.size;
    }
    printf("%lf\n",temp_value);
    MPI_Gather(&temp_value, 1, MPI_DOUBLE, &average_for_nodes, 1, MPI_DOUBLE, root, root_comm.comm);
    MPI_Gather(name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, &node_names, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, root, root_comm.comm);
    if(world_comm.rank == root){
          for(k=0; k<root_comm.size; k++){
        printf("avg for nodes %lf\n", average_for_nodes[k]);
         } 
    }
  }

  printf("bb\n");

  iloc.value = indivi.max;
  iloc.rank = world_comm.rank;
  MPI_Allreduce(&iloc, &rloc, 1, MPI_DOUBLE_INT, MPI_MAXLOC, world_comm.comm);
  if(rloc.rank == world_comm.rank && rloc.value != indivi.max){
    printf("Error with the output of MPI_MAXLOC reduction");
  }
  result->max = rloc.value;
  // Communicate which node has the biggest max value so outlier nodes can be identified
  if(rloc.rank == world_comm.rank && rloc.rank != root){
    MPI_Ssend(name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, root, 0, world_comm.comm);
  }else if(world_comm.rank == root && rloc.rank != root){
    MPI_Recv(max_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, rloc.rank, 0, world_comm.comm, &status);
  }else if(world_comm.rank == root && rloc.rank == root){
    strcpy(max_name, name);
  }

  printf("cc\n");

  // Get the total max value across all processes in a node for each repeat of the benchmark to enable calculation
  // of the minimum and maximum bandwidth seen within a node. The minimum bandwidth will be the longest time 
  // for all processes across all the repeats of the benchmark (i.e. that represents the maximum runtime for 
  // any of the repeats). The maximum bandwidth will be the minimum longest time across the repeats of the 
  // benchmark (i.e. the repeat of the benchmark that takes the shortest overall time). We can use Max here 
  // because each benchmark is surrounded by Barriers meaning that the longest process for each run 
  // represents the slowest part of that run and therefore the limit on the bandwith achieved.
  max_time_store = 0;
  min_time_store = FLT_MAX;
  for(k=1; k<NTIMES; k++) {
    temp_value = indivi.raw_result[k];
    MPI_Reduce(&temp_value, &temp_result, 1, MPI_DOUBLE, MPI_MAX, root, node_comm.comm);
    if(temp_result > max_time_store){
      max_time_store = temp_result;
    }
    if(temp_result < min_time_store){
      min_time_store = temp_result;
    }
  }

  node_result->max = max_time_store;
  node_result->min = min_time_store;

  printf("dd\n");

  // Get the total max and min value across all the nodes
  // For the max we want the slowest node (i.e. the MPI_MAX of the max)
  // For the min we want the fastest node (i.e. the MPI_MIN of the min)
  // These should give us the upper and lower bounds on the node performances
  if(node_comm.rank == root){
    temp_value = node_result->max;
    MPI_Reduce(&temp_value, &temp_result, 1, MPI_DOUBLE, MPI_MAX, root, root_comm.comm);
    if(world_comm.rank == root){
      node_result->max = temp_result;
    }
    MPI_Gather(&temp_value, 1, MPI_DOUBLE, &max_for_nodes, 1, MPI_DOUBLE, root, root_comm.comm);
    temp_value = node_result->min;
    MPI_Reduce(&temp_value, &temp_result, 1, MPI_DOUBLE, MPI_MIN, root, root_comm.comm);
    if(world_comm.rank == root){
      node_result->min = temp_result;
    }
    MPI_Gather(&temp_value, 1, MPI_DOUBLE, &min_for_nodes, 1, MPI_DOUBLE, root, root_comm.comm);

  }

  printf("ee\n");

  iloc.value = indivi.min;
  iloc.rank = world_comm.rank;
  MPI_Allreduce(&iloc, &rloc, 1, MPI_DOUBLE_INT, MPI_MINLOC, world_comm.comm);
  result->min = rloc.value;

  printf("ff\n");

  if(node_comm.rank == root){
    switch (benchmark){
    case copy:
      for(k=0; k<root_comm.size; k++){
	all_node_results[k].Copy.avg = average_for_nodes[k];
	all_node_results[k].Copy.max = max_for_nodes[k];
	all_node_results[k].Copy.min = min_for_nodes[k];
      }
      break;
    case scale:
      for(k=0; k<root_comm.size; k++){
	all_node_results[k].Scale.avg = average_for_nodes[k];
	all_node_results[k].Scale.max = max_for_nodes[k];
	all_node_results[k].Scale.min = min_for_nodes[k];
      }
      break;
    case add:
      for(k=0; k<root_comm.size; k++){
	all_node_results[k].Add.avg = average_for_nodes[k];
	all_node_results[k].Add.max = max_for_nodes[k];
	all_node_results[k].Add.min = min_for_nodes[k];
      }
      break;
    case triad:
      for(k=0; k<root_comm.size; k++){
	all_node_results[k].Triad.avg = average_for_nodes[k];
	all_node_results[k].Triad.max = max_for_nodes[k];
	all_node_results[k].Triad.min = min_for_nodes[k];
      }
      break;
    default:
      break;
    }
    for(k=0; k<root_comm.size; k++){
      strcpy(all_node_results[k].name, node_names[k]);
    }
  }

  printf("gg\n");

}

// Initialise the benchmark results structure to enable proper collection of data
void initialise_benchmark_results(benchmark_results *b_results){

  int name_length;

  b_results->Copy.avg = 0;
  b_results->Copy.min = FLT_MAX;
  b_results->Copy.max= 0;
  b_results->Copy.raw_result = malloc(NTIMES * sizeof(double));
  b_results->Scale.avg = 0;
  b_results->Scale.min = FLT_MAX;
  b_results->Scale.max= 0;
  b_results->Scale.raw_result = malloc(NTIMES * sizeof(double));
  b_results->Add.avg = 0;
  b_results->Add.min = FLT_MAX;
  b_results->Add.max= 0;
  b_results->Add.raw_result = malloc(NTIMES * sizeof(double));
  b_results->Triad.avg = 0;
  b_results->Triad.min = FLT_MAX;
  b_results->Triad.max= 0;
  b_results->Triad.raw_result = malloc(NTIMES * sizeof(double));
  MPI_Get_processor_name(b_results->name, &name_length);

}

// Initialise the benchmark results structure to enable proper collection of data
void free_benchmark_results(benchmark_results *b_results){

  free(b_results->Copy.raw_result);
  free(b_results->Scale.raw_result);
  free(b_results->Add.raw_result);
  free(b_results->Triad.raw_result);

}

// Print out aggregate results. The intention is that this will only
// be called from the root process as the overall design is that
// only the root process (the process which has ROOT rank) will
// have this data.
void print_results(aggregate_results a_results, aggregate_results node_results, communicator world_comm, int array_size, communicator node_comm){

  int omp_num_threads;
  double bandwidth_avg, bandwidth_max, bandwidth_min;
  double copy_size = 2 * sizeof(STREAM_TYPE) * array_size;
  double scale_size = 2 * sizeof(STREAM_TYPE) * array_size;
  double add_size	= 3 * sizeof(STREAM_TYPE) * array_size;
  double triad_size = 3 * sizeof(STREAM_TYPE) * array_size;


#pragma omp parallel default(shared)
  {
    omp_num_threads = omp_get_num_threads();
  }
  printf("Running with %d MPI processes, each with %d OpenMP threads. %d processes per node\n", world_comm.size, omp_num_threads, node_comm.size);
  printf("Benchmark   Average Bandwidth    Avg Time    Max Bandwidth   Min Time    Min Bandwidth   Max Time   Max Time Location\n");
  printf("                  (MB/s)         (seconds)       (MB/s)      (seconds)       (MB/s)      (seconds)      (proc name)\n");
  printf("----------------------------------------------------------------------------------------------------------------------\n");

  // Calculate the bandwidths. Max bandwidth is achieved using the min time (i.e. the fast time). This is
  // why max and min are opposite either side of the "=" below
  bandwidth_avg = (1.0E-06 * copy_size)/a_results.Copy.avg;
  bandwidth_max = (1.0E-06 * copy_size)/a_results.Copy.min;
  bandwidth_min = (1.0E-06 * copy_size)/a_results.Copy.max;
  printf("Copy:     %12.1f:   %11.6f:  %12.1f:   %11.6f:   %12.1f:   %11.6f   %s\n", bandwidth_avg, a_results.Copy.avg, bandwidth_max, a_results.Copy.min, bandwidth_min, a_results.Copy.max, a_results.copy_max);

  // Calculate the bandwidths. Max bandwidth is achieved using the min time (i.e. the fast time). This is
  // why max and min are opposite either side of the "=" below
  bandwidth_avg = (1.0E-06 * scale_size)/a_results.Scale.avg;
  bandwidth_max = (1.0E-06 * scale_size)/a_results.Scale.min;
  bandwidth_min = (1.0E-06 * scale_size)/a_results.Scale.max;
  printf("Scale:    %12.1f:   %11.6f:  %12.1f:   %11.6f:   %12.1f:   %11.6f   %s\n", bandwidth_avg, a_results.Scale.avg, bandwidth_max, a_results.Scale.min, bandwidth_min, a_results.Scale.max, a_results.scale_max);

  // Calculate the bandwidths. Max bandwidth is achieved using the min time (i.e. the fast time). This is
  // why max and min are opposite either side of the "=" below
  bandwidth_avg = (1.0E-06 * add_size)/a_results.Add.avg;
  bandwidth_max = (1.0E-06 * add_size)/a_results.Add.min;
  bandwidth_min = (1.0E-06 * add_size)/a_results.Add.max;
  printf("Add:      %12.1f:   %11.6f:  %12.1f:   %11.6f:   %12.1f:   %11.6f   %s\n", bandwidth_avg, a_results.Add.avg, bandwidth_max, a_results.Add.min, bandwidth_min, a_results.Add.max, a_results.add_max);

  // Calculate the bandwidths. Max bandwidth is achieved using the min time (i.e. the fast time). This is
  // why max and min are opposite either side of the "=" below
  bandwidth_avg = (1.0E-06 * triad_size)/a_results.Triad.avg;
  bandwidth_max = (1.0E-06 * triad_size)/a_results.Triad.min;
  bandwidth_min = (1.0E-06 * triad_size)/a_results.Triad.max;
  printf("Triad:    %12.1f:   %11.6f:  %12.1f:   %11.6f:   %12.1f:   %11.6f   %s\n", bandwidth_avg, a_results.Triad.avg, bandwidth_max, a_results.Triad.min, bandwidth_min, a_results.Triad.max, a_results.triad_max);

  // Calculate the node bandwidths.
  bandwidth_avg = ((1.0E-06 * copy_size * node_comm.size)/node_results.Copy.avg);
  bandwidth_max = ((1.0E-06 * copy_size * node_comm.size)/node_results.Copy.min);
  bandwidth_min = ((1.0E-06 * copy_size * node_comm.size)/node_results.Copy.max);
  printf("Node Copy:  %12.1f:   %11.6f:  %12.1f:   %11.6f:   %12.1f:   %11.6f\n", bandwidth_avg, node_results.Copy.avg, bandwidth_max, node_results.Copy.min, bandwidth_min, node_results.Copy.max);

  // Calculate the node bandwidths.
  bandwidth_avg = ((1.0E-06 * scale_size * node_comm.size)/node_results.Scale.avg);
  bandwidth_max = ((1.0E-06 * scale_size * node_comm.size)/node_results.Scale.min);
  bandwidth_min = ((1.0E-06 * scale_size * node_comm.size)/node_results.Scale.max);
  printf("Node Scale: %12.1f:   %11.6f:  %12.1f:   %11.6f:   %12.1f:   %11.6f\n", bandwidth_avg, node_results.Scale.avg, bandwidth_max, node_results.Scale.min, bandwidth_min, node_results.Scale.max);

  // Calculate the node bandwidths.
  bandwidth_avg = ((1.0E-06 * add_size * node_comm.size)/node_results.Add.avg);
  bandwidth_max = ((1.0E-06 * add_size * node_comm.size)/node_results.Add.min);
  bandwidth_min = ((1.0E-06 * add_size * node_comm.size)/node_results.Add.max);
  printf("Node Add:   %12.1f:   %11.6f:  %12.1f:   %11.6f:   %12.1f:   %11.6f\n", bandwidth_avg, node_results.Add.avg, bandwidth_max, node_results.Add.min, bandwidth_min, node_results.Add.max);

  // Calculate the node bandwidths.
  bandwidth_avg = ((1.0E-06 * triad_size * node_comm.size)/node_results.Triad.avg);
  bandwidth_max = ((1.0E-06 * triad_size * node_comm.size)/node_results.Triad.min);
  bandwidth_min = ((1.0E-06 * triad_size * node_comm.size)/node_results.Triad.max);
  printf("Node Triad: %12.1f:   %11.6f:  %12.1f:   %11.6f:   %12.1f:   %11.6f\n", bandwidth_avg, node_results.Triad.avg, bandwidth_max, node_results.Triad.min, bandwidth_min, node_results.Triad.max);

  return;

}

// Save to file node results. The intention is that this will only
// be called from the root process as the overall design is that
// only the root process (the process which has ROOT rank) will
// have this data.
void save_results(char *filename, benchmark_results *all_node_results, int array_size, communicator world_comm, communicator node_comm, communicator root_comm){

  FILE *fp;
  mxml_node_t *tree;
  mxml_node_t *xml; 
  mxml_node_t *node;
  mxml_node_t *hardware;
  mxml_node_t *experiment;
  mxml_node_t *results;
  mxml_node_t *result;
  mxml_node_t *individual_result;

  int k, omp_num_threads;
  long copy_size = 2 * sizeof(STREAM_TYPE) * array_size;
  long scale_size = 2 * sizeof(STREAM_TYPE) * array_size;
  long add_size = 3 * sizeof(STREAM_TYPE) * array_size;
  long triad_size = 3 * sizeof(STREAM_TYPE) * array_size;

#pragma omp parallel default(shared)
  {
    omp_num_threads = omp_get_num_threads();
  }

  xml = mxmlNewXML("1.0");
  tree = mxmlNewElement(xml, "stream_run");

  experiment = mxmlNewElement(tree, "experiment");
  mxmlNewText(experiment, 0, filename);

  hardware = mxmlNewElement(tree, "configuration");
  node = mxmlNewElement(hardware, "processes_per_node");
  mxmlNewInteger(node, node_comm.size);
  node = mxmlNewElement(hardware, "threads_per_process");
  mxmlNewInteger(node, omp_num_threads);
  node =  mxmlNewElement(hardware, "number_of_nodes");
  mxmlNewInteger(node, root_comm.size);
  node =  mxmlNewElement(hardware, "copy_size");
  mxmlNewInteger(node, copy_size);
  node =  mxmlNewElement(hardware, "scale_size");
  mxmlNewInteger(node, scale_size);
  node =  mxmlNewElement(hardware, "add_size");
  mxmlNewInteger(node, add_size);
  node =  mxmlNewElement(hardware, "triad_size");
  mxmlNewInteger(node, triad_size);



  results = mxmlNewElement(tree, "results");
  for(k=0; k<root_comm.size; k++){
    result = mxmlNewElement(results, "node");
    node = mxmlNewElement(result, "name");
    mxmlNewText(node, 0, all_node_results[k].name);
    node = mxmlNewElement(result, "Copy");
    individual_result = mxmlNewElement(node, "Average");
    mxmlNewReal(individual_result, all_node_results[k].Copy.avg);
    individual_result = mxmlNewElement(node, "Minimum");
    mxmlNewReal(individual_result, all_node_results[k].Copy.min);
    individual_result = mxmlNewElement(node, "Maximum");
    mxmlNewReal(individual_result, all_node_results[k].Copy.max);
    node = mxmlNewElement(result, "Scale");
    individual_result = mxmlNewElement(node, "Average");
    printf("Scale average %lf\n", all_node_results[k].Scale.avg);
    mxmlNewReal(individual_result, all_node_results[k].Scale.avg);
    individual_result = mxmlNewElement(node, "Minimum");
    mxmlNewReal(individual_result, all_node_results[k].Scale.min);
    individual_result = mxmlNewElement(node, "Maximum");
    mxmlNewReal(individual_result, all_node_results[k].Scale.max);
    node = mxmlNewElement(result, "Add");
    individual_result = mxmlNewElement(node, "Average");
    mxmlNewReal(individual_result, all_node_results[k].Add.avg);
    individual_result = mxmlNewElement(node, "Minimum");
    mxmlNewReal(individual_result, all_node_results[k].Add.min);
    individual_result = mxmlNewElement(node, "Maximum");
    mxmlNewReal(individual_result, all_node_results[k].Add.max);
    node = mxmlNewElement(result, "Triad");
    individual_result = mxmlNewElement(node, "Average");
    mxmlNewReal(individual_result, all_node_results[k].Triad.avg);
    individual_result = mxmlNewElement(node, "Minimum");
    mxmlNewReal(individual_result, all_node_results[k].Triad.min);
    individual_result = mxmlNewElement(node, "Maximum");
    mxmlNewReal(individual_result, all_node_results[k].Triad.max);
  }
  

  fp = fopen(filename, "w");
  mxmlSaveFile(xml, fp, MXML_NO_CALLBACK);
  fclose(fp);
  
  mxmlDelete(xml);

  return;

}
