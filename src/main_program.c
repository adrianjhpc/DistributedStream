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

#pragma omp parallel default(shared) private(omp_thread_num)
	{
		omp_thread_num = omp_get_thread_num();
		printf("MPI Rank: %d OpenMP Thread: %d Name %s\n", prank, omp_thread_num, b_results.name);
	}

	//stream_task(&b_results);
	//collect_results(b_results, &a_results);

	MPI_Finalize();

}

void collect_results(benchmark_results b_results, aggregate_results *a_results){


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
