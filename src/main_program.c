#include "definitions.h"

int main(int argc, char **argv){

	int prank, psize;
	int node_size, node_rank;
	int node_comm, node_key;
	int omp_thread_num;
	int array_size;
	benchmark_results b_results;
	aggregate_results node_results;
	aggregate_results a_results;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &psize);
	MPI_Comm_rank(MPI_COMM_WORLD, &prank);

	// Get a integer key for this process that is different for every node
	// a process is run on.
	node_key = get_key();

	// Use the node key to split the MPI_COMM_WORLD communicator
	// to produce a communicator per node, containing all the processes
	// running on a given node.
	MPI_Comm_split(MPI_COMM_WORLD,node_key,0,&node_comm);

	// Get the rank and size of the node communicator this process is involved
	// in.
	MPI_Comm_size(node_comm, &node_size);
	MPI_Comm_rank(node_comm, &node_rank);

	initialise_benchmark_results(&b_results);

	stream_memory_task(&b_results, psize, prank, node_size, &array_size);
	collect_results(b_results, &a_results, &node_results, psize, prank, node_comm, node_size, node_rank);

	if(prank == ROOT){
		print_results(a_results, psize, array_size, node_size);
	}

	/*initialise_benchmark_results(&b_results);

        stream_persistent_memory_task(&b_results, psize, prank, node_size, &array_size);
        collect_results(b_results, &a_results, &node_results, psize, prank, node_comm, node_size, node_rank);

        if(prank == ROOT){
		printf("Stream Persistent Memory Results");
                print_results(a_results, psize, array_size, node_size);
        }
	 */

	MPI_Finalize();

}

void collect_results(benchmark_results b_results, aggregate_results *a_results, aggregate_results *node_results, int psize, int prank, int node_comm, int node_size, int node_rank){

	collect_individual_result(b_results.Copy, &a_results->Copy, &node_results->Copy, a_results->copy_max, psize, prank, b_results.name, node_comm, node_size, node_rank);
	collect_individual_result(b_results.Scale, &a_results->Scale, &node_results->Scale, a_results->scale_max, psize, prank, b_results.name, node_comm, node_size, node_rank);
	collect_individual_result(b_results.Add, &a_results->Add, &node_results->Add, a_results->add_max, psize, prank, b_results.name, node_comm, node_size, node_rank);
	collect_individual_result(b_results.Triad, &a_results->Triad, &node_results->Triad, a_results->triad_max, psize, prank, b_results.name, node_comm, node_size, node_rank);

}

void collect_individual_result(performance_result indivi, performance_result *result, performance_result *node_result, char *max_name, int psize, int prank, char *name, int node_comm, int node_size, int node_rank){

	// Structure to hold both a value and a rank for MAXLOC and MINLOC operations.
	// This *may* be problematic on some MPI implementations as it assume MPI_DOUBLE_INT
	// matches this specification.
	typedef struct resultloc {
		double value;
		int   rank;
	} resultloc;

	double temp_value;
	double temp_result;

	// Variable for the result of the reduction
	resultloc rloc;
	// Variable for the data to be reduced
	resultloc iloc;

	int root = ROOT;
	MPI_Status status;

	MPI_Reduce(&indivi.avg, &result->avg, 1, MPI_DOUBLE, MPI_SUM, root, MPI_COMM_WORLD);
	if(prank == root){
		result->avg = result->avg/psize;
	}

	// Get the total avg value summed across all processes in a node to enable calculation
	// of the avg bandwidth for a node.
	temp_value = indivi.avg;
	MPI_Reduce(&temp_value, &temp_result, 1, MPI_DOUBLE, MPI_SUM, ROOT, node_comm);
	node_result->avg = temp_result;

	iloc.value = indivi.max;
	iloc.rank = prank;
	MPI_Allreduce(&iloc, &rloc, 1, MPI_DOUBLE_INT, MPI_MAXLOC, MPI_COMM_WORLD);
	if(rloc.rank == prank && rloc.value != indivi.max){
		printf("Error with the output of MPI_MAXLOC reduction");
	}
	result->max = rloc.value;
	// Communicate which node has the biggest max value so outlier nodes can be identified
	if(rloc.rank == prank && rloc.rank != root){
		MPI_Ssend(name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, root, 0, MPI_COMM_WORLD);
	}else if(prank == root && rloc.rank != root){
		MPI_Recv(max_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, rloc.rank, 0, MPI_COMM_WORLD, &status);
	}else if(rloc.rank == root){
		strcpy(max_name, name);
	}

	// Get the total max value summed across all processes in a node to enable calculation
	// of the minimum bandwidth for a node.
	temp_value = iloc.value;
	MPI_Reduce(&temp_value, &temp_result, 1, MPI_DOUBLE, MPI_SUM, ROOT, node_comm);
	node_result->max = temp_result;

	iloc.value = indivi.min;
	iloc.rank = prank;
	MPI_Allreduce(&iloc, &rloc, 1, MPI_DOUBLE_INT, MPI_MINLOC, MPI_COMM_WORLD);
	result->min = rloc.value;

	// Get the total min value summed across all processes in a node to enable calculation
	// of the maximum bandwidth for a node.
	temp_value = iloc.value;
	MPI_Reduce(&temp_value, &temp_result, 1, MPI_DOUBLE, MPI_SUM, ROOT, node_comm);
	node_result->min = temp_result;

}

// Initialise the benchmark results structure to enable proper collection of data
void initialise_benchmark_results(benchmark_results *b_results){

	int name_length;

	b_results->Copy.avg = 0;
	b_results->Copy.min = FLT_MAX;
	b_results->Copy.max= 0;
	b_results->Scale.avg = 0;
	b_results->Scale.min = FLT_MAX;
	b_results->Scale.max= 0;
	b_results->Add.avg = 0;
	b_results->Add.min = FLT_MAX;
	b_results->Add.max= 0;
	b_results->Triad.avg = 0;
	b_results->Triad.min = FLT_MAX;
	b_results->Triad.max= 0;

	MPI_Get_processor_name(b_results->name, &name_length);

}

// Print out aggregate results. The intention is that this will only
// be called from the root process as the overall design is that
// only the root process (the process which has ROOT rank) will
// have this data.
void print_results(aggregate_results a_results, int psize, int array_size, int processes_per_node){

	int omp_thread_num;
	double bandwidth_avg, bandwidth_max, bandwidth_min;
	double copy_size = 2 * sizeof(STREAM_TYPE) * array_size;
	double scale_size = 2 * sizeof(STREAM_TYPE) * array_size;
	double add_size	= 3 * sizeof(STREAM_TYPE) * array_size;
	double triad_size = 3 * sizeof(STREAM_TYPE) * array_size;


#pragma omp parallel default(shared)
	{
		omp_thread_num = omp_get_num_threads();
	}

	printf("Running with %d MPI processes, each with %d OpenMP threads. %d processes per node\n", psize, omp_thread_num, processes_per_node);
	printf("Benchmark   Average Bandwidth    Avg Time    Max Bandwidth   Min Time    Min Bandwidth   Max Time   Max Time Location\n");
	printf("                  (GB/s)         (seconds)       (GB/s)      (seconds)       (GB/s)      (seconds)      (proc name)\n");
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

	// Calculate the node bandwidths. These are the bandwidths calculated above multiplied by the number of
	// processes per node
	bandwidth_avg = ((1.0E-06 * copy_size)/a_results.Copy.avg)*processes_per_node;
	bandwidth_max = ((1.0E-06 * copy_size)/a_results.Copy.min)*processes_per_node;
	bandwidth_min = ((1.0E-06 * copy_size)/a_results.Copy.max)*processes_per_node;
	printf("Node Copy:  %12.1f:   %11.6f:  %12.1f:   %11.6f:   %12.1f:   %11.6f   %s\n", bandwidth_avg, a_results.Copy.avg, bandwidth_max, a_results.Copy.min, bandwidth_min, a_results.Copy.max, a_results.copy_max);

	// Calculate the node bandwidths. These are the bandwidths calculated above multiplied by the number of
	// processes per node
	bandwidth_avg = ((1.0E-06 * scale_size)/a_results.Scale.avg)*processes_per_node;
	bandwidth_max = ((1.0E-06 * scale_size)/a_results.Scale.min)*processes_per_node;
	bandwidth_min = ((1.0E-06 * scale_size)/a_results.Scale.max)*processes_per_node;
	printf("Node Scale: %12.1f:   %11.6f:  %12.1f:   %11.6f:   %12.1f:   %11.6f   %s\n", bandwidth_avg, a_results.Scale.avg, bandwidth_max, a_results.Scale.min, bandwidth_min, a_results.Scale.max, a_results.scale_max);

	// Calculate the node bandwidths. These are the bandwidths calculated above multiplied by the number of
	// processes per node
	bandwidth_avg = ((1.0E-06 * add_size)/a_results.Add.avg)*processes_per_node;
	bandwidth_max = ((1.0E-06 * add_size)/a_results.Add.min)*processes_per_node;
	bandwidth_min = ((1.0E-06 * add_size)/a_results.Add.max)*processes_per_node;
	printf("Node Add:   %12.1f:   %11.6f:  %12.1f:   %11.6f:   %12.1f:   %11.6f   %s\n", bandwidth_avg, a_results.Add.avg, bandwidth_max, a_results.Add.min, bandwidth_min, a_results.Add.max, a_results.add_max);

	// Calculate the node bandwidths. These are the bandwidths calculated above multiplied by the number of
	// processes per node
	bandwidth_avg = ((1.0E-06 * triad_size)/a_results.Triad.avg)*processes_per_node;
	bandwidth_max = ((1.0E-06 * triad_size)/a_results.Triad.min)*processes_per_node;
	bandwidth_min = ((1.0E-06 * triad_size)/a_results.Triad.max)*processes_per_node;
	printf("Node Triad: %12.1f:   %11.6f:  %12.1f:   %11.6f:   %12.1f:   %11.6f   %s\n", bandwidth_avg, a_results.Triad.avg, bandwidth_max, a_results.Triad.min, bandwidth_min, a_results.Triad.max, a_results.triad_max);

}

#if defined(__aarch64__)
// TODO: This might be general enough to provide the functionality for any system
// regardless of processor type given we aren't worried about thread/process migration.
// Test on Intel systems and see if we can get rid of the architecture specificity
// of the code.
unsigned long get_processor_and_core(int *chip, int *core){
	return syscall(SYS_getcpu, &core, &chip, NULL);
}
// TODO: Add in AMD function
#else
// If we're not on an ARM processor assume we're on an intel processor and use the
// rdtscp instruction.
unsigned long get_processor_and_core(int *chip, int *core)
{
	unsigned long a,d,c;
	__asm__ volatile("rdtscp" : "=a" (a), "=d" (d), "=c" (c));
	*chip = (c & 0xFFF000)>>12;
	*core = c & 0xFFF;
	return ((unsigned long)a) | (((unsigned long)d) << 32);;
}
#endif

int name_to_colour(const char *name){
	int res;
	const char *p;
	res =0;
	for(p=name; *p ; p++){
		res = (131*res) + *p;
	}

	if( res < 0 ){
		res = -res;
	}
	return res;
}

// Get an integer key for a process based on the name of the
// node this process is running on. This is useful for creating
// communicators for all the processes running on a node.
int get_key(){

	char name[MPI_MAX_PROCESSOR_NAME];
	int len;
	int lpar_key;
	int rank;

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Get_processor_name(name, &len);
	lpar_key = name_to_colour(name);

	return lpar_key;

}
