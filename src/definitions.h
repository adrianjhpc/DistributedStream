#include <mpi.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <float.h>
#include <string.h>
#include <sys/sysinfo.h>
#if defined(__aarch64__)
#include <sys/syscall.h>
#endif

#ifndef STREAM_TYPE
#define STREAM_TYPE double
#endif

# ifndef MIN
# define MIN(x,y) ((x)<(y)?(x):(y))
# endif
# ifndef MAX
# define MAX(x,y) ((x)>(y)?(x):(y))
# endif

/*  STREAM runs each kernel "NTIMES" times and reports the *best* result
 *         for any iteration after the first, therefore the minimum value
 *         for NTIMES is 2.
 *      There are no rules on maximum allowable values for NTIMES, but
 *         values larger than the default are unlikely to noticeably
 *         increase the reported performance.
 *      NTIMES can also be set on the compile line without changing the source
 *         code using, for example, "-DNTIMES=7".
 */
#ifdef NTIMES
#if NTIMES<=1
#   define NTIMES	10
#endif
#endif
#ifndef NTIMES
#   define NTIMES	10
#endif

#define ROOT 0
#define MAX_FILE_NAME_LENGTH 500

typedef struct communicator {
	MPI_Comm comm;
	int rank;
	int size;
} communicator;

typedef struct raw_result {
	double copy;
	double scale;
	double add;
	double triad;
} raw_result;

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
	char copy_max[MPI_MAX_PROCESSOR_NAME];
	char scale_max[MPI_MAX_PROCESSOR_NAME];
	char add_max[MPI_MAX_PROCESSOR_NAME];
	char triad_max[MPI_MAX_PROCESSOR_NAME];
} aggregate_results;

int get_key();
int stream_memory_task(benchmark_results *b_results, raw_result *r_results, communicator world_comm, communicator node_comm, int *array_size);
int stream_persistent_memory_task(benchmark_results *b_results, raw_result *r_results, communicator world_comm, communicator node_comm, int *array_size);
void collect_results(benchmark_results result, raw_result *r_results, aggregate_results *agg_result, aggregate_results *node_results, communicator world_comm, communicator node_comm, communicator root_comm);
void initialise_benchmark_results(benchmark_results *b_results, raw_result **r_results);
void collect_individual_result(performance_result indivi, raw_result *r_result, performance_result *result, performance_result *node_result, char *max_name, char *name, communicator world_comm, communicator node_comm, communicator root_comm);
void print_results(aggregate_results a_results, raw_result *r_results, aggregate_results node_results, communicator world_comm, int array_size, communicator node_comm);
