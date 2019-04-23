#include "definitions.h"

int main(int argc, char **argv){

	int prank, psize;
	int node_size, node_rank;
	char procname[MPI_MAX_PROCESSOR_NAME];
	int node_comm, node_key;
	int omp_thread_num;
	int array_size;
	benchmark_results b_results;
	aggregate_results a_results;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &psize);
	MPI_Comm_rank(MPI_COMM_WORLD, &prank);

	node_key = get_key(procname);

	MPI_Comm_split(MPI_COMM_WORLD,node_key,0,&node_comm);

	MPI_Comm_size(node_comm, &node_size);
	MPI_Comm_rank(node_comm, &node_rank);

	initialise_benchmark_results(&b_results);

	stream_memory_task(&b_results, psize, prank, node_size, &array_size);
	collect_results(b_results, &a_results, psize, prank);

	if(prank == ROOT){
		print_results(a_results, psize, array_size, node_size);
	}

	/*initialise_benchmark_results(&b_results);

        stream_persistent_memory_task(&b_results, psize, prank, node_size, &array_size);
        collect_results(b_results, &a_results, psize, prank);

        if(prank == ROOT){
		printf("Stream Persistent Memory Results");
                print_results(a_results, psize, array_size, node_size);
        }
	 */

	MPI_Finalize();

}

void collect_results(benchmark_results b_results, aggregate_results *a_results, int psize, int prank){

	collect_individual_result(b_results.Copy, &a_results->Copy, a_results->copy_max, psize, prank, b_results.name);
	collect_individual_result(b_results.Scale, &a_results->Scale, a_results->scale_max, psize, prank, b_results.name);
	collect_individual_result(b_results.Add, &a_results->Add, a_results->add_max, psize, prank, b_results.name);
	collect_individual_result(b_results.Triad, &a_results->Triad, a_results->triad_max, psize, prank, b_results.name);

}

void collect_individual_result(performance_result indivi, performance_result *result, char *max_name, int psize, int prank, char *name){

	// Structure to hold both a value and a rank for MAXLOC and MINLOC operations.
	// This *may* be problematic on some MPI implementations as it assume MPI_DOUBLE_INT
	// matches this specification.
	typedef struct resultloc {
		double value;
		int   rank;
	} resultloc;

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
	iloc.value = indivi.max;
	iloc.rank = prank;
	MPI_Allreduce(&iloc, &rloc, 1, MPI_DOUBLE_INT, MPI_MAXLOC, MPI_COMM_WORLD);
	if(rloc.rank == prank && rloc.value != indivi.max){
		printf("Error with the output of MPI_MAXLOC reduction");
	}
	result->max = rloc.value;
	if(rloc.rank == prank && rloc.rank != root){
		MPI_Ssend(name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, root, 0, MPI_COMM_WORLD);
	}else if(prank == root && rloc.rank != root){
		MPI_Recv(max_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR, rloc.rank, 0, MPI_COMM_WORLD, &status);
	}else if(rloc.rank == root){
		strcpy(max_name, name);
	}
	iloc.value = indivi.min;
	iloc.rank = prank;
	MPI_Allreduce(&iloc, &rloc, 1, MPI_DOUBLE_INT, MPI_MINLOC, MPI_COMM_WORLD);
	result->min = rloc.value;

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

int get_key(char *name){

	int len;
	int lpar_key;
	int cpu;
	int core;

	MPI_Get_processor_name(name, &len);
	printf("name: %s",name);
	get_processor_and_core(&cpu,&core);
	name = name + cpu;
	lpar_key = name_to_colour(name);

	lpar_key = cpu + lpar_key;

	return lpar_key;

}
