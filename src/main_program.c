#include "definitions.h"

int main(int argc, char **argv){

	int prank, psize, name_length;
	int omp_thread_num;
	struct benchmark_results b_results;
	struct aggregate_results a_results;

	MPI_Init(&argc, &argv);
	MPI_Comm_size(MPI_COMM_WORLD, &psize);
	MPI_Comm_rank(MPI_COMM_WORLD, &prank);

	MPI_Get_processor_name(&b_results.name, &name_length);

#pragma omp parallel default(shared) private(omp_thread_num){
	printf("MPI Rank: %d OpenMP Thread: %d Name %s\n", prank, omp_thread_num, b_results.name);
}

	stream_task(&b_results);
	collect_results(b_results, &a_results);

	MPI_Finalize();

}

void collect_results(struct benchmark_results b_results, struct aggregate_results *a_results){


}
