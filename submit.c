/* MPI code for a work scheduler */
/* Last process is the master, that receives requests from the workers
   and assigns them work */

/* Author: Martin Cuma, CHPC, University of Utah, mcuma@chpc.utah.edu */
   

#include "submit.h"
#include <unistd.h>

#ifdef USE_VT
#include "VT.h"
void VT_setup(void);            /* VAMPIRtrace instrumentation setup */
#endif

/*  ******  Start of Main routine  **********************************   */

int main(int argc, char **argv)
{
/* variable declarations */

int numprocs,myid,server,active_procs;
/* Request is a 2 int array, one = request itself, 2 = previous event termination 
   flag */
int work=1, ret_event=0;
int i, err;
work_data_struct work_data;

char outfilename[100];

MPI_Comm world;
MPI_Datatype work_data_type;

MPI_Status status;

/* Initialize MPI and find out procs, who I am */
#ifdef _OPENMP
int thread_status;
MPI_Init_thread(&argc, &argv,MPI_THREAD_MULTIPLE,&thread_status);
if (thread_status!=MPI_THREAD_MULTIPLE)
{
 printf("Failed to initialize MPI_THREAD_MULTIPLE\n");
 exit(-1);
}
#else
  MPI_Init(&argc, &argv);
#endif
world = MPI_COMM_WORLD;
MPI_Comm_size(world,&numprocs);
MPI_Comm_rank(world,&myid);

/* Process command line arguments */
/* note that this does not work with PGI compilers and Icebox's MPICH compiled with gcc */
/* first argument - filename containing jobs to run */
#ifndef _OPENMP
  if (numprocs<2)
  {
    printf("Need to run at least 2 MPI tasks in non OpenMP mode\n");
    return 1;
  }
#endif
printf("[%d] Argument count %d, %s\n",myid,argc,argv[1]);

if (myid == 0)
{
/* first argument - in and out directory */
  if (argc == 2)
  {
     strcpy(EVENTIN_STR,EVENTIN_DEF);
     strcpy(in_dir,argv[1]);
     strcpy(out_dir,argv[1]);
  }
/* second argument - in directory in case out directory is different */
  else if (argc == 3)
  {
     strcpy(EVENTIN_STR,EVENTIN_DEF);
     strcpy(in_dir,argv[1]);
     strcpy(out_dir,argv[2]);
  }
  else
/* if no command line argument - take defaults from submit.h */
  {
     strcpy(EVENTIN_STR,EVENTIN_DEF);
     strcpy(out_dir,OUTDIR_STR);
     strcpy(in_dir,INDIR_STR);
  }
}
/* now we must bcast all the arguments to other nodes, as only node 0 reads them */
/* ideally length would be strlen(bin_dir+1), but, then, would also have to bcast
   the lengths, which is too much work */
MPI_Bcast(&in_dir, 255, MPI_CHAR, 0,  world);
MPI_Bcast(&out_dir, 255, MPI_CHAR, 0,  world);
MPI_Bcast(&EVENTIN_STR, 255, MPI_CHAR, 0,  world);

     printf("[%d] outdir assigned %s\n",myid,out_dir);
     printf("[%d] indir assigned %s\n",myid,in_dir);
     printf("[%d] EVENTIN_STR assigned %s\n",myid,EVENTIN_STR);


/* Last proc. is the server */
server = numprocs - 1;
active_procs = numprocs - 1;

/* Set output to stdout - only this works on Raptor */
/* output = stdout; */

/* Build MPI derived data type to send the event info structure */
build_work_data_DDT(&work_data, &work_data_type);

if (myid == server)
{
/* ******************************************************************** */
/* Server part - collect requests and distribute work */

/* handles to file database input and output */
	FILE* eventin;
	FILE* eventout;
	work_data_struct* work_array;
	int num_tasks;
/* Array that flags finished job */
/* 0 = not started, 2 = started, 1 = finished */
	int* job_flag;
/* time it took to run each event */
	double* timer;

/* define std. output file for the server */


	sprintf(outfilename,"%s/%s",OUTDIR_STR,"master.out");
        if ( (output = fopen(outfilename, "w")) == NULL)
        {
          printf("Failed to open info output file %s",outfilename);
          MPI_Abort(world,1);
        }

        fprintf(output,"Input dir is %s\n",in_dir);
        fprintf(output,"Output dir is %s\n",out_dir);

/* Open and read in the event database */
        sprintf(outfilename,"%s/%s",in_dir,EVENTIN_STR);
        fprintf(output,"Event database is %s\n",outfilename);

        if ( (eventin = fopen(outfilename, "rt")) == NULL)
	{
	   printf("Can't open the event database file %s\n",outfilename);
	   MPI_Abort(world,1);
	}          
        if ( (num_tasks = read_events(eventin, &work_array, &job_flag, &timer)) < 0)
	{
           printf("Event database file %s has wrong format\n",outfilename);
           MPI_Abort(world,1);
	}
	fclose(eventin);

/* Initialize few things */
        work_data.event_id = 0;

/* Open the event finalization status file */
/* REM - there is no control if the child process crashes */
/*       must recover this info from the text outputs */
        sprintf(outfilename,"%s/%s",out_dir,EVENTOUT_STR);
        if ( (eventout = fopen(outfilename,"w")) == NULL)
        {
          printf("Failed to open event output file %s",outfilename);
          MPI_Abort(world,1);
        }


	fprintf(output,"Number of events to run: %d\n",num_tasks);
	for (i=0;i<num_tasks;i++)
	   fprintf(output,"Initial job_flag %d %d\n",i,job_flag[i]);


/***** Here starts the OMP thread distribution ******/
#ifdef _OPENMP
omp_set_num_threads(2);
#endif

#ifdef _OPENMP
#pragma omp parallel sections private(work) 
#endif
{
#ifdef _OPENMP
fprintf(output,"OMPs threads %d %d\n",omp_get_num_threads(),omp_get_max_threads()); 
#endif

/* Master thread section - communicates with the remote slaves */
#ifdef _OPENMP
#pragma omp section
#endif
{
      int request[2];
#ifdef _OPENMP
      fprintf(output,"Here is the OMP master thread %d \n",omp_get_thread_num()); 
#endif
      if (numprocs>1) /* if only have 1 proc, then can't get requests from other workers */
      {
	do
	{
/* Receive request for work */
		MPI_Recv(&request, 2, MPI_INT, MPI_ANY_SOURCE, REQUEST, world, &status);
/* request[0] = event_id from the just finished event, or -1 if requesting first 
   event */
/* request[1] = finished event termination flag, zero if correct finish */

		if (request[0] >= 0)
		{
/* Print event completion report */
/* after input event database, print the whole event info */
/* Critical section here so the slave thread can write too */
#ifdef _OPENMP
#pragma omp critical (write_out)
#endif
{
  		   job_flag[request[0]] = request[1];
                   timer[request[0]] = gettime() - timer[request[0]];
                   print_event_info(eventout,request,job_flag,work_array,timer[request[0]],status.MPI_SOURCE);
                   fflush(output);
                   fflush(eventout);
}
		}

/* Find the work */
/* OMP - here must be critical section */
#ifdef _OPENMP
#pragma omp critical (gen_work)
#endif
{
		work = generate_event(&work_data, num_tasks, work_array,
 					job_flag);
}

/* Send the work flag to the destination */
/* may optimize to send work flag and work data together in one message */
		MPI_Send(&work, 1, MPI_INT, status.MPI_SOURCE, REPLY, world);
/* If have work, send work data to the process */
		if (work)
		{
			MPI_Send(&work_data, 1, work_data_type, status.MPI_SOURCE, 
				REPLY, world);
/* OMP - another critical section here - with different name */
#ifdef _OPENMP
#pragma omp critical (upd_work)
#endif
{
                        timer[work_data.event_id] = gettime();
			work_data.event_id++;
}
		}
/* Otherwise decrease the number of active processors by one */
		else
		{
		   active_procs--;
                   fprintf(output,"Remaining active processors: %d\n",active_procs);
		}
	} while (active_procs);
     }
     fprintf(output,"OMP master exiting\n");

/* End OMP master thread section */
}

#ifdef _OPENMP
/* Worker thread of the master processor */
#pragma omp section
{
      int request[2];
      work_data_struct work_sl_data;
      fprintf(output,"Here is the OMP worker thread %d \n",omp_get_thread_num()); 

/* The slave thread also calls the generate event, but with private work_data
   - one problem with this is that work_data.event_id is updated every time
     the work data pool is accessed 
*/
        ret_event = 0;

        do
        {
                sleep(1);
#pragma omp critical (gen_work)
{ 
	        work_sl_data.event_id = work_data.event_id;
        	work = generate_event(&work_sl_data, num_tasks, work_array,
                                        job_flag);
              fprintf(output,"OMP slave crit. sect. %d %d 1\n",work_sl_data.event_id,work); 
}

	        if (work)
        	{

#pragma omp critical (upd_work)
{
                        timer[work_sl_data.event_id] = gettime();
	        	work_data.event_id++;
}

/* Now the OMP slave thread does the work */
                       ret_event = do_work(work_sl_data, myid);
                       request[0] = work_sl_data.event_id;
                       request[1] = ret_event;
                       fprintf(output,"Event %d on slave exited with status %d\n",
                        work_sl_data.event_id,ret_event);

/* Slave writes output in the single eventout file */
#pragma omp critical (write_out)
{
		       job_flag[request[0]] = ret_event;
                       timer[work_sl_data.event_id] = gettime() - timer[work_sl_data.event_id];
                       print_event_info(eventout,request,job_flag,work_array,timer[work_sl_data.event_id],myid);
                       fflush(output);
                       fflush(eventout);
}

		}
/* And exits when there is no work left */
        } while (work);

/* else it exits and waits for the master to finish, too */
          fprintf(output,"OMP slave exiting\n");
/* End OMP slave thread section */
}
#endif
/* End OMP sections */
}
#ifdef _OPENMP
#pragma omp barrier
#endif

        fclose(eventout);
        free_events(num_tasks,work_array, job_flag);

}
else
/* ****************************************************************** */

/* workers - send work requests and receive work */
{
/* define std. output file for the server */

      int request[2];

        sprintf(outfilename,"%s/work%02d.out",OUTDIR_STR,myid);
        if ( (output = fopen(outfilename,"w")) == NULL)
        {
          printf("Failed to open info output file %s",outfilename);
          MPI_Abort(world,1);
        }

	work_data.event_id = -1;
        ret_event = 0;
	do
	{
		request[0] = work_data.event_id;
                request[1] = ret_event; 
/* worker process asks server for work */
                fprintf(output,"Process %d sending request for work\n",myid);
		MPI_Send(&request, 2, MPI_INT, server, REQUEST, world);

/* and receives info from server if there is a work for it */
		MPI_Recv(&work, 1, MPI_INT, server, REPLY, world, &status);

                fprintf(output,"Process %d received work %d\n",myid,work);

/* if there is, perform the work, if not, we are done and jump out of the do loop */
		if (work)
		{
	   	   MPI_Recv(&work_data, 1, work_data_type, server, REPLY, world, &status);
		   ret_event = do_work(work_data, myid);
/* Gotta send the ret_event back to the master so it knows if the processing
   finished correctly 
*/
                   fprintf(output,"Event %d exited with status %d\n",
			work_data.event_id,ret_event);
		}
	} while (work);
}

MPI_Type_free(&work_data_type);

/* fprintf(output,"Process %d waiting at barrier\n",myid); */
fprintf(output,"Process %d exited\n\n\n",myid);

fclose(output);

err = MPI_Barrier(world);

printf("[%d] Last barrier return %d\n",myid,err);


MPI_Finalize();

}
