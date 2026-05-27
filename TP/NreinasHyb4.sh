#!/bin/bash
#SBATCH -N 2
#SBATCH --exclusive
#SBATCH --partition=Blade
#SBATCH --tasks-per-node=1
#SBATCH -o SalidasHyb4/output.txt
#SBATCH -e SalidasHyb4/errors.txt
#SBATCH --time=00:10:00 #Tiempo límite (HH:MM:SS)
mpirun --bind-to none nreinasHyb $1 $2