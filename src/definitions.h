#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef struct performance_result {
   double avg;
   double min;
   double max;
} performance_result;

typedef struct benchmark_results {
	struct performance_result Copy;
	struct performance_result Scale;
	struct performance_result Add;
	struct performance_result Triad;
	char name[MPI_MAX_PROCESSOR_NAME];
} benchmark_results;

typedef struct aggregate_results {
	struct performance_result Copy;
	struct performance_result Scale;
	struct performance_result Add;
	struct performance_result Triad;
	char copy_min[MPI_MAX_PROCESSOR_NAME];
	char scale_min[MPI_MAX_PROCESSOR_NAME];
	char add_min[MPI_MAX_PROCESSOR_NAME];
	char triad_min[MPI_MAX_PROCESSOR_NAME];
} aggregate_results;

int stream(benchmark_results *b_result);
void collect_results(benchmark_results result, aggregate_results *agg_result);
void initialise_benchmark_results(benchmark_results *b_results);

