#include "definitions.h"

int main(int argc, char **argv){

	int temp_size, temp_rank;
	MPI_Comm temp_comm;
	int node_key;
	int omp_thread_num;
	int array_size;
	benchmark_results b_results;
	raw_result *r_results;
	aggregate_results node_results;
	aggregate_results a_results;
	communicator world_comm, node_comm, root_comm;

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

	initialise_benchmark_results(&b_results, r_results);

	stream_memory_task(&b_results, r_results, world_comm, node_comm, &array_size);
	collect_results(b_results, r_results, &a_results, &node_results, world_comm, node_comm, root_comm);

	if(world_comm.rank == ROOT){
		print_results(a_results, r_results, node_results, world_comm, array_size, node_comm);
	}

	/*initialise_benchmark_results(&b_results, r_results);

        stream_persistent_memory_task(&b_results, r_results, world_comm, node_comm, &array_size);
        collect_results(b_results, &r_results, &a_results, &node_results, world_comm, node_comm, root_comm);

        if(world_comm.rank == ROOT){
		printf("Stream Persistent Memory Results");
                print_results(a_results, node_results, world_comm, array_size, node_comm);
        }
	 */

	free(r_results);

	MPI_Finalize();

}

void collect_results(benchmark_results b_results, raw_results *r_results, aggregate_results *a_results, aggregate_results *node_results, communicator world_comm, communicator node_comm, communicator root_comm){

	collect_individual_result(b_results.Copy, &a_results->Copy, &node_results->Copy, a_results->copy_max, b_results.name, world_comm, node_comm, root_comm);
	collect_individual_result(b_results.Scale, &a_results->Scale, &node_results->Scale, a_results->scale_max,  b_results.name, world_comm, node_comm, root_comm);
	collect_individual_result(b_results.Add, &a_results->Add, &node_results->Add, a_results->add_max, b_results.name, world_comm, node_comm, root_comm);
	collect_individual_result(b_results.Triad, &a_results->Triad, &node_results->Triad, a_results->triad_max, b_results.name, world_comm, node_comm, root_comm);

}

void collect_individual_result(performance_result indivi, performance_result *result, performance_result *node_result, char *max_name, char *name, communicator world_comm, communicator node_comm, communicator root_comm){

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

	MPI_Reduce(&indivi.avg, &result->avg, 1, MPI_DOUBLE, MPI_SUM, root, world_comm.comm);
	if(world_comm.rank == root){
		result->avg = result->avg/world_comm.size;
	}

	// Get the total avg value summed across all processes in a node to enable calculation
	// of the avg bandwidth for a node.
	temp_value = indivi.avg;
	MPI_Reduce(&temp_value, &temp_result, 1, MPI_DOUBLE, MPI_SUM, ROOT, node_comm.comm);
	node_result->avg = temp_result;

	if(node_comm.rank == root){
		temp_value = node_result->avg;
		MPI_Reduce(&temp_value, &temp_result, 1, MPI_DOUBLE, MPI_SUM, ROOT, root_comm.comm);
		if(world_comm.rank == root){
			node_result->avg = temp_result/root_comm.size;
		}
	}

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
	}else if(rloc.rank == root){
		strcpy(max_name, name);
	}

	// Get the total max value summed across all processes in a node to enable calculation
	// of the minimum bandwidth for a node.
	temp_value = iloc.value;
	MPI_Reduce(&temp_value, &temp_result, 1, MPI_DOUBLE, MPI_SUM, ROOT, node_comm.comm);
	node_result->max = temp_result;

	// Get the total max value across all the nodes
	if(node_comm.rank == root){
		temp_value = node_result->max;
		MPI_Reduce(&temp_value, &temp_result, 1, MPI_DOUBLE, MPI_MAX, ROOT, root_comm.comm);
		if(world_comm.rank == root){
			node_result->max = temp_result;
		}
	}

	iloc.value = indivi.min;
	iloc.rank = world_comm.rank;
	MPI_Allreduce(&iloc, &rloc, 1, MPI_DOUBLE_INT, MPI_MINLOC, world_comm.comm);
	result->min = rloc.value;

	// Get the total min value summed across all processes in a node to enable calculation
	// of the maximum bandwidth for a node.
	temp_value = iloc.value;
	MPI_Reduce(&temp_value, &temp_result, 1, MPI_DOUBLE, MPI_SUM, ROOT, node_comm.comm);
	node_result->min = temp_result;

	// Get the total min value across all the nodes
	if(node_comm.rank == root){
		temp_value = node_result->min;
		MPI_Reduce(&temp_value, &temp_result, 1, MPI_DOUBLE, MPI_MIN, ROOT, root_comm.comm);
		if(world_comm.rank == root){
			node_result->min = temp_result;
		}
	}


}

// Initialise the benchmark results structure to enable proper collection of data
void initialise_benchmark_results(benchmark_results *b_results, raw_result *r_result){

	int name_length;

	r_result = malloc(NTIMES * sizeof(r_result));

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
void print_results(aggregate_results a_results, raw_result *r_results, aggregate_results node_results, communicator world_comm, int array_size, communicator node_comm){

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
	printf("Running with %d MPI processes, each with %d OpenMP threads. %d processes per node\n", world_comm.size, omp_thread_num, node_comm.size);
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

	// Calculate the node bandwidths.
	bandwidth_avg = ((1.0E-06 * copy_size * node_comm.size)/node_results.Copy.avg) * node_comm.size;
	bandwidth_max = ((1.0E-06 * copy_size * node_comm.size)/node_results.Copy.min) * node_comm.size;
	bandwidth_min = ((1.0E-06 * copy_size * node_comm.size)/node_results.Copy.max) * node_comm.size;
	printf("Node Copy:  %12.1f:   %11.6f:  %12.1f:   %11.6f:   %12.1f:   %11.6f\n", bandwidth_avg, node_results.Copy.avg, bandwidth_max, node_results.Copy.min, bandwidth_min, node_results.Copy.max);

	// Calculate the node bandwidths.
	bandwidth_avg = ((1.0E-06 * scale_size * node_comm.size)/node_results.Scale.avg) * node_comm.size;
	bandwidth_max = ((1.0E-06 * scale_size * node_comm.size)/node_results.Scale.min) * node_comm.size;
	bandwidth_min = ((1.0E-06 * scale_size * node_comm.size)/node_results.Scale.max) * node_comm.size;
	printf("Node Scale: %12.1f:   %11.6f:  %12.1f:   %11.6f:   %12.1f:   %11.6f\n", bandwidth_avg, node_results.Scale.avg, bandwidth_max, node_results.Scale.min, bandwidth_min, node_results.Scale.max);

	// Calculate the node bandwidths.
	bandwidth_avg = ((1.0E-06 * add_size * node_comm.size)/node_results.Add.avg) * node_comm.size;
	bandwidth_max = ((1.0E-06 * add_size * node_comm.size)/node_results.Add.min) * node_comm.size;
	bandwidth_min = ((1.0E-06 * add_size * node_comm.size)/node_results.Add.max) * node_comm.size;
	printf("Node Add:   %12.1f:   %11.6f:  %12.1f:   %11.6f:   %12.1f:   %11.6f\n", bandwidth_avg, node_results.Add.avg, bandwidth_max, node_results.Add.min, bandwidth_min, node_results.Add.max);

	// Calculate the node bandwidths.
	bandwidth_avg = ((1.0E-06 * triad_size * node_comm.size)/node_results.Triad.avg) * node_comm.size;
	bandwidth_max = ((1.0E-06 * triad_size * node_comm.size)/node_results.Triad.min) * node_comm.size;
	bandwidth_min = ((1.0E-06 * triad_size * node_comm.size)/node_results.Triad.max) * node_comm.size;
	printf("Node Triad: %12.1f:   %11.6f:  %12.1f:   %11.6f:   %12.1f:   %11.6f\n", bandwidth_avg, node_results.Triad.avg, bandwidth_max, node_results.Triad.min, bandwidth_min, node_results.Triad.max);
}

// The routine convert a string (name) into a number
// for use in a MPI_Comm_split call (where the number is
// known as a colour). It is effectively a hashing function
// for strings but is not necessarily robust (i.e. does not
// guarantee it is collision free) for all strings, but it
// should be reasonable for strings that different by small
// amounts (i.e the name of nodes where they different by a
// number of set of numbers and letters, for instance
// login01,login02..., or cn01q94,cn02q43, etc...)
int name_to_colour(const char *name){
	int res;
	int multiplier = 131;
	const char *p;

	res = 0;
	for(p=name; *p ; p++){
		// Guard against integer overflow.
		if (multiplier > 0 && (res + *p) > (INT_MAX / multiplier)) {
			// If overflow looks likely (due to the calculation above) then
			// simply flip the result to make it negative
			res = -res;
		}else{
			// If overflow is not going to happen then undertake the calculation
			res = (multiplier*res);
		}
		// Add on the new character here.
		res = res + *p;
	}
	// If we have ended up with a negative result, invert it to make it positive because
	// the functionality (in MPI) that we will use this for requires the int to be positive.
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

	MPI_Get_processor_name(name, &len);
	lpar_key = name_to_colour(name);

	return lpar_key;

}
