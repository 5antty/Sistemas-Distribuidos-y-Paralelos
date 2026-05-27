#!/bin/bash
#SBATCH -N 2
#SBATCH --exclusive
#SBATCH --partition=Blade
#SBATCH --tasks-per-node=8
#SBATCH -o SalidasHyb16/output.txt
#SBATCH -e SalidasHyb16/errors.txt
#SBATCH --time=00:10:00 #Tiempo límite (HH:MM:SS)
mpirun --bind-to none nreinasHyb16 $1 $2