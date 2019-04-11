#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

struct performance_result {
   double avg;
   double min;
   double max;
};

struct benchmark_results {
	struct performance_result Copy = {0, FLT_MAX, 0};
	struct performance_result Scale = {0, FLT_MAX, 0};
	struct performance_result Add = {0, FLT_MAX, 0};
	struct performance_result Triad = {0, FLT_MAX, 0};
	char name[MPI_MAX_PROCESSOR_NAME];
};

struct aggregate_results {
	struct performance_result Copy;
	struct performance_result Scale;
	struct performance_result Add;
	struct performance_result Triad;
	char copy_min[MPI_MAX_PROCESSOR_NAME];
	char scale_min[MPI_MAX_PROCESSOR_NAME];
	char add_min[MPI_MAX_PROCESSOR_NAME];
	char triad_min[MPI_MAX_PROCESSOR_NAME];

};

int stream(struct benchmark_results *b_result);
void collect_results(struct benchmark_results result, struct benchmark_results *agg_result);
