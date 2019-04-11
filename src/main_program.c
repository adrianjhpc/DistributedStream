#include "definitions.h"

int main(int argc, char **argv){

	int prank, psize;
	int omp_thread_num;
	benchmark_results b_results;
	aggregate_results a_results;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &psize);
	MPI_Comm_rank(MPI_COMM_WORLD, &prank);

	initialise_benchmark_results(&b_results);

	stream_memory_task(&b_results);
	collect_results(b_results, &a_results, psize, prank);

	if(prank == ROOT){
		print_results(a_results, psize);
	}

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
		max_name = name;
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
void print_results(aggregate_results a_results, int psize){

	int omp_thread_num;
	double bandwidth_avg, bandwidth_max, bandwidth_min;
	double copy_size = 2 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE;
	double scale_size = 2 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE;
	double add_size	= 3 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE;
	double triad_size = 3 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE;


#pragma omp parallel default(shared)
	{
		omp_thread_num = omp_get_num_threads();
	}

	printf("Running with %d MPI processes, each with %d OpenMP threads\n", psize, omp_thread_num);
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


}
