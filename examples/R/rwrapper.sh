#!/bin/bash 

# NOTE:
#   EXE : rwapper.sh 
#   TASK_ID     : Id of the task
#   SCRATCH_DIR : EACH task has its own scratch directory
#   SCRIPT_DIR  : Script is identical for each task => Same directory for ALL tasks
#   OUT_DIR     : Output directory is identical for each task

# Retrieve variables from the command line
START_DIR=$PWD
EXE=$0
TASK_ID=$1
SCRATCH_DIR=$2
SCRIPT_DIR=$3
OUT_DIR=$4

if [ "$#" -ne 4 ] ; then
     echo "  ERROR: Command line needs 4 parameters"
     echo "  Current arg list: $@"
else
     echo "  TaskID:$TASK_ID started at `date`"
     mkdir -p $SCRATCH_DIR  
     cd $SCRATCH_DIR
     # Copy content SCRIPT_DIR to SCRATCH_DIR
     cp -pR $SCRIPT_DIR/* . 
     Rscript mcex.R $1 >& rscript$1.out
     rm *.R

     # Copy results back to OUT_DIR
     mkdir -p $OUT_DIR
     cp -pR * $OUT_DIR
     cd $START_DIR
     rm -rf $SCRATCH_DIR
     echo "  TaskID:$TASK_ID ended at `date`"
fi
