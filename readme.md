#Submit 

##Purpose

The `submit` program allows to run many serial calculations inside of a parallel cluster job using master-worker model. The program is a simple MPI based scheduler which reads a list of calculations to do from a file, one per line, and runs them in parallel, filling as many calculations as there are parallel tasks. If one calculation is finished, the worker asks for another calculation, which keeps repeating until all calculations are done.

This approach is useful for running many shorter independent calculations which vary in compute time. Longer calculations would be better served by the Open Science Grid (OSG). If some calculations take much more time than others, as `submit` is not very intelligent, order the calculations in the list file by the longest being the first, if possible.

##Installation

To build the code, just type `make` in the main directory. It will build the executable, test, and run a simple test. The `Makefile` is set up for Intel compiler and Intel MPI, but it should work with any OpenMP capable compiler and MPI distribution - just be aware that most MPI distributions use polling when waiting for messages which puts an extra load on the idle master process - this is why we use `I_MPI_WAIT_MODE=1` for Intel MPI. 

##Usage

The executable is submitted like an MPI job, e.g.:
```
mpirun -genv I_MPI_WAIT_MODE 1 -np $SLURM_NTASKS path_to/submit
```

The submit program takes in input file called `job.list`, which syntax is as follows:
first line - # of serial jobs to run
other lines - command line for these serial jobs (including program arguments). Make sure there is only single space between the program arguments - more that single space will break the command line.

For example (for testing purpose), you can make `job.list` as:
```
4
/bin/hostname -i
/bin/hostname -i
/bin/hostname -i
/bin/hostname -i
```

This will run 4 serial jobs, executing `hostname` command - which returns name of the node this command run on.
NOTE - make sure to use full path to the command

##Examples

In the `tests` directory is a simple test example.

In the `examples` directory are real examples.

### Many serial R calculations

are located in the `examples/R` directory. `sim_submit.sh` is a shell script which creates the `job.list` and runs the simulations, companion SLURM script that does the same thing in a job is in `sim_submit.sl`.
