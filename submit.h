/* MPI code for a work scheduler */
/* common header file    */

#include <stdio.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>

#include "ctimer.h"

/*VAMPIRtrace defines */
/* #define USE_VT */

#define MAX_STRING 255

/* Tags for request and reply messages */
#define REQUEST 1
#define REPLY 2

/* Process invocation defines */
#define MAX_ARG_LEN 255
#define ARG_CNT 20

/* here indir is the data directory, outdir is output directory */
/*
#define INDIR_STR "/uufs/genepi/sys/src/Uinfo9_mc"
#define OUTDIR_STR "/uufs/genepi/sys/src/Uinfo9_mc"
*/
#define INDIR_STR "."
#define OUTDIR_STR "."

#define EVENTIN_DEF "job.list"
char EVENTIN_STR[MAX_STRING];
#define EVENTOUT_STR "job.out"

/* Structure used to send the data to the worker */
typedef struct 
{
  int event_id;
  int arg_cnt;
/* arg must be static because we use it to define derived data type */
  char arg[MAX_ARG_LEN];
} work_data_struct;

/* output file - global declaration - will not work on the SGI */
FILE* output;
char in_dir[MAX_STRING];
char out_dir[MAX_STRING];
char bin_dir[MAX_STRING];

int read_events(FILE* eventin, work_data_struct** work_array, int** job_flag, double** timer);
int free_events(int num_tasks, work_data_struct* work_array, int* job_flag);

int do_work(work_data_struct work_data,int myid);
int generate_event(work_data_struct *work_data, int num_tasks,
	work_data_struct* work_array, int* job_flag);
void print_event_info(FILE* eventout,int* request,int* job_flag,
	work_data_struct* work_array, double timer, int worker);
int parse_line(char* buf,char** argline);

void build_work_data_DDT(work_data_struct* work_data, MPI_Datatype* work_data_type);

