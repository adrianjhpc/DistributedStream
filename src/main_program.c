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

	collect_individual_result(b_results.Copy, &a_results->Copy, psize, prank);
	collect_individual_result(b_results.Scale, &a_results->Scale, psize, prank);
	collect_individual_result(b_results.Add, &a_results->Add, psize, prank);
	collect_individual_result(b_results.Triad, &a_results->Triad, psize, prank);

}

void collect_individual_result(performance_result indivi, performance_result *result, int psize, int prank){


    typedef struct resultloc {
        double value;
        int   rank;
    } resultloc;

    resultloc rloc;

	int root = ROOT;

	MPI_Reduce(&indivi.avg, &result->avg, 1, MPI_DOUBLE, MPI_SUM, root, MPI_COMM_WORLD);
	if(prank == root){
		result->avg = result->avg/psize;
	}
	MPI_Reduce(&indivi.max, &rloc, 1, MPI_DOUBLE_INT, MPI_MAXLOC, root, MPI_COMM_WORLD);
	result->max = rloc.value;
	MPI_Reduce(&indivi.min, &rloc, 1, MPI_DOUBLE_INT, MPI_MINLOC, root, MPI_COMM_WORLD);
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
	double total_data;
	double copy_size = 2 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE;
	double scale_size = 2 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE;
	double add_size	= 3 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE;
	double triad_size = 3 * sizeof(STREAM_TYPE) * STREAM_ARRAY_SIZE;


#pragma omp parallel default(shared)
	{
		omp_thread_num = omp_get_num_threads();
	}

	printf("Running with %d MPI processes, each with %d OpenMP threads\n", psize, omp_thread_num);
	printf("Benchmark:   Achieved Bandwidth:    Avg Time:   Min Time:  Max Time:\n");

	total_data = (1.0E-06 * copy_size)/a_results.Copy.min;
	printf("Copy:  %12.1f:   %11.6f:   %11.6f:   %11.6f\n", total_data, a_results.Copy.avg, a_results.Copy.min, a_results.Copy.max);

	total_data = (1.0E-06 * scale_size)/a_results.Copy.min;
	printf("Scale: %12.1f:   %11.6f:   %11.6f:   %11.6f\n", total_data, a_results.Scale.avg, a_results.Scale.min, a_results.Scale.max);

	total_data = (1.0E-06 * add_size)/a_results.Add.min;
	printf("Add:   %12.1f:   %11.6f:   %11.6f:   %11.6f\n", total_data, a_results.Add.avg, a_results.Add.min, a_results.Add.max);

	total_data = (1.0E-06 * triad_size)/a_results.Triad.min;
	printf("Triad: %12.1f:   %11.6f:   %11.6f:   %11.6f\n", total_data, a_results.Triad.avg, a_results.Triad.min, a_results.Triad.max);


}
