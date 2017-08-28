/* MPI code for a work scheduler */

#include "submit.h"
/* These are headers used to invoke another process */
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>


/* *************************************************************** 
Subroutines   
   *************************************************************** */


int do_work(work_data_struct work_data,int myid)
{
int i,j;
char* argstr[ARG_CNT+2];
char *comm_str;
int status;
double ttime;
pid_t pid;

fprintf(output,"Process %d performing event %d\n", myid,
        work_data.event_id);


fprintf(output,"arguments %s\n",work_data.arg);

ttime = gettime();

for(i=0;i<(ARG_CNT+2);i++)
   argstr[i] = NULL;

/* work_data.arg_cnt=parse_line(work_data.arg,argstr+1); 
*/

  pid = fork ();
  if (pid == 0)
    {
      /* This is the child process.  Execute the shell command. */
      /* here comes the parsing */
        work_data.arg_cnt=parse_line(work_data.arg,argstr);

        comm_str = (char*) malloc(strlen(argstr[0])+2);
        sprintf(comm_str,"%s",argstr[0]);
        printf("comm_str %s %s %s %s %s\n",comm_str,argstr[1],argstr[2],argstr[3],argstr[4]);
//        printf("arg_cnt %d\n",work_data.arg_cnt);

        execv(comm_str, argstr);
//      char *cmd[] = { argstr[0] , argstr[1],  (char *)0 };
//      execv(comm_str, cmd);

        for(j=0;j<(ARG_CNT+2);j++)
        {
          if (argstr[j] != NULL)
            free(argstr[j]);
        }
        free(comm_str);

        _exit (EXIT_FAILURE);
    }
  else if (pid < 0)
    {
    /* The fork failed.  Report failure.  */
    status = -2;
    fprintf(output,"Fork failed, process %d \n",myid);
    }
  else
    /* This is the parent process.  Wait for the child to complete.  */
/*    wait (&status); */
      waitpid (pid, &status, 0);
/*      if (waitpid (pid, &status, 0) != pid)   
        status = -1;  */ 

   fprintf(output,"Event %d done\n",work_data.event_id);

   fprintf(output,"Pid %d, Statuses %d %d %d\n",pid,status,EXIT_FAILURE,WEXITSTATUS(status));
      ttime = gettime() - ttime;

   fprintf(output,"Runtime %f\n",ttime);
   fflush(output);

   return WEXITSTATUS(status);

}

/* Generates new job, returns argument line length if there is a new job
   or 0 it there is not */
int generate_event(work_data_struct* work_data, int num_tasks, 
	work_data_struct* work_array, int* job_flag)
{
int ev_no,arglen;

ev_no = work_data->event_id;

if ((ev_no) < num_tasks)
{
	job_flag[ev_no] = 127;
        
        work_data->arg_cnt = work_array[ev_no].arg_cnt;
        arglen = strlen(work_array[ev_no].arg)+1;

        strcpy(work_data->arg,work_array[ev_no].arg);

	fprintf(output,"%d tasks processed, job flag %d, %s\n", 
                work_data->event_id, 
		job_flag[work_data->event_id],
                work_data->arg); 
	return arglen;
}
else
{
/*	fprintf(output,"Terminating task\n"); */
	return 0;
} 
}

int read_events(FILE* eventin,work_data_struct** work_array, int** job_flag, double** timer)
{
int nevents,i=0;
char buf[MAX_STRING],*Ptr;

/* Read in the number of events - first line in the file */
fgets( buf, (MAX_STRING-1) , eventin);
nevents = atoi(buf);

/* Allocate memory for the work data and job flag arrays */
(*job_flag) = (int*)malloc(nevents*sizeof(int));
(*timer) = (double*)malloc(nevents*sizeof(double));
(*work_array) = (work_data_struct*)malloc(nevents*sizeof(work_data_struct));

/* Loop over the events and read in data from the file */
/* for (i=0;i<nevents;i++)
{
  (*job_flag)[i]=-1;
*/

/* Sometime incorporate possibility of commenting out a line */
for( ; ; )
{
/*  nacitej radky az do konce souboru */

        if( fgets( buf, (MAX_STRING-1) , eventin) == NULL )
        {
                break;
        }
/*   vyfiltruj konce radku a komentare (znaky  "##") */

/*        printf("orig buff %sxx\n",buf); */
        if( (Ptr = strpbrk( buf, "#\n\r")) != NULL )
        {
                if( (*Ptr     != '#')
                ||  (*(Ptr+1) == '#') )
                {
                        *Ptr = '\0';
                }
        }
/*        printf("buffer %sxx\n",buf); */
        if( buf[0] == '\0' )
        {
                continue;
        }

/* if we read more than nevents lines, break */
        if (i > nevents) break;

        strcpy((*work_array)[i].arg,buf);
        (*timer)[i]=0.0;
        (*job_flag)[i++]=-1;
fprintf(output,"Scanned data %d: %s\n",i-1,
           (*work_array)[i-1].arg);

}


/*  fgets( (*work_array)[i].arg, (MAX_STRING-1) , eventin); */


return nevents;
}

int free_events(int num_tasks,work_data_struct* work_array, int* job_flag)
{

free(work_array);
free(job_flag);

return 0;
}

void print_event_info(FILE* eventout,int* request,int* job_flag,
	work_data_struct* work_array, double timer, int worker)
{

        fprintf(output,"Event %d finished on worker %d with flag %d time %f\n",
                        request[0],worker,job_flag[request[0]],timer);
	fprintf(eventout,"Event %d finished on worker %d with flag %d time %f\n",
                        request[0],worker,job_flag[request[0]],timer);
        fprintf(output,"Event data: %s\n",
                     work_array[request[0]].arg);
 	fprintf(eventout,"Event data: %s\n",
                     work_array[request[0]].arg);


return;

}

int parse_line(char* buf,char** argline)
{
	char* Ptr;
        int i=0;

        for( ; ; )
        {
                if( buf[0] == '\0' )
                {
                        break;
                }
/* split the buffer into two strings, " " is the delimeter */

                if( (Ptr = strpbrk( buf, " ")) == NULL )
                {
/* last argument */
	                argline[i] = (char*)malloc(strlen(buf)+1);
        	        strcpy(argline[i++],buf);
/*                	printf("Last Argument %d is %sxx\n",i-1,argline[i-1]);  */
                	break;
                }

                *(Ptr++)='\0';


                argline[i] = (char*)malloc(strlen(buf)+1);
                strcpy(argline[i++],buf);
/*                printf("Argument %d is %sxx\n",i-1,argline[i-1]); */
                buf = Ptr;
        }

	return i;
}


void build_work_data_DDT(work_data_struct* work_data, MPI_Datatype* work_data_type)
{
/* size of W_DATA_TYPES will be work_data->arg_cnt+2 */
/* now hardcoded since don't know the arg_cnt before the run */
#define W_DATA_TYPES 3

int block_lengths[W_DATA_TYPES],i;
MPI_Aint displacements[W_DATA_TYPES];
MPI_Datatype typelist[W_DATA_TYPES];

MPI_Aint start_address;
MPI_Aint address;

block_lengths[0] = block_lengths[1] = 1;
typelist[0] = typelist[1] = MPI_INT;

displacements[0] = 0;

MPI_Get_address(&(work_data->event_id),&start_address);

MPI_Get_address(&(work_data->arg_cnt),&address);
displacements[1] = address - start_address;


/* loop over work_data->arg_cnt */
block_lengths[2] = MAX_ARG_LEN;
typelist[2] = MPI_CHAR;
MPI_Get_address(&(work_data->arg),&address);
displacements[2] = address - start_address;

MPI_Type_struct(W_DATA_TYPES, block_lengths, displacements, typelist, work_data_type);
MPI_Type_commit(work_data_type);

}

