#!/bin/bash

export NTASKS=4

export EXE=./rwrapper.sh
export WORK_DIR=/uufs/chpc.utah.edu/common/home/u0101881/submit/submit/examples/R
export SCRIPT_DIR=$WORK_DIR/Rfiles
export OUT_DIR=$WORK_DIR/results
export SCRATCH_DIR=$WORK_DIR

# Load R (version 3.3.2)
module load R/3.3.2
module load impi # intel is already loaded from R

# Run an array of serial jobs
export OMP_NUM_THREADS=1

# initial counter for the jobs to allow multiple jobs
echo $NTASKS > job.list
START_COUNTER=1
for (( i=0; i < $NTASKS ; i++ )); do  
   ip=$((START_COUNTER+i))
   echo $EXE $ip $SCRATCH_DIR/$ip $SCRIPT_DIR $OUT_DIR ; \
done >> job.list

# Run a task on each core
#cd $WORK_DIR
mpirun -genv I_MPI_WAIT_MODE 1  -np 4 ../../bin/submit

# Clean-up the root scratch dir
#rm -rf $SCRATCH_DIR
