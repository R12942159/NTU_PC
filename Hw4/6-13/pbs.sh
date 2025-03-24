#!/bin/bash
#PBS -N ESOE7045
#PBS -l walltime=00:05:00
#PBS -l select=1:ncpus=8:mpiprocs=8
export FI_PROVIDER=tcp
cd $PBS_O_WORKDIR
usecpus=`cat $PBS_NODEFILE |wc -l`
module load tbb/2021.13 compiler-rt/2024.2.0 compiler/2024.2.0 mpi/2021.13 mkl/2024.2
program=./main
matrix_bin=matrix_10_by_10.bin
iteration=10
freq=1
mpiexec.hydra -f $PBS_NODEFILE -n $usecpus -ppn 1 $program $matrix_bin $iteration $freq
