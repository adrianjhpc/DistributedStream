#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

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

typedef enum {
	none,
	individual,
	collective
} persist_state;

typedef struct communicator {
	MPI_Comm comm;
	int rank;
	int size;
} communicator;

typedef struct performance_result {
	double avg;
	double min;
	double max;
	double *raw_result;
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

typedef enum {
  copy,
  scale,
  add,
  triad
} benchmark_type;

int stream_memory_task(benchmark_results *b_results, communicator world_comm, communicator node_comm, int *array_size);
#ifdef PMEM
int stream_memkind_memory_task(benchmark_results *b_results, communicator world_comm, communicator node_comm, int *array_size, int socket);
int stream_persistent_memory_task(benchmark_results *b_results, communicator world_comm, communicator node_comm, int *array_size, int socket, persist_state persist_level);
int stream_write_persistent_memory_task(benchmark_results *b_results, communicator world_comm, communicator node_comm, int *array_size, int socket, persist_state persist_level);
int stream_read_persistent_memory_task(benchmark_results *b_results, communicator world_comm, communicator node_comm, int *array_size, int socket);
#endif
void collect_results(benchmark_results result, aggregate_results *agg_result, aggregate_results *node_results, benchmark_results *all_node_results, communicator world_comm, communicator node_comm, communicator root_comm);
void initialise_benchmark_results(benchmark_results *b_results);
void free_benchmark_results(benchmark_results *b_results);
void collect_individual_result(performance_result indivi, performance_result *result, performance_result *node_result, char *max_name, char *name, benchmark_results *all_node_results, benchmark_type benchmark, communicator world_comm, communicator node_comm, communicator root_comm);
void print_results(aggregate_results a_results, aggregate_results node_results, communicator world_comm, int array_size, communicator node_comm);
void save_results(char *filename, benchmark_results *all_node_results, int array_size, communicator world_comm, communicator node_comm, communicator root_comm);
