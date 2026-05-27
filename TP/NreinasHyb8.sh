#!/bin/bash
#SBATCH -N 2
#SBATCH --exclusive
#SBATCH --partition=Blade
#SBATCH --tasks-per-node=4
#SBATCH -o SalidasHyb8/output.txt
#SBATCH -e SalidasHyb8/errors.txt
#SBATCH --time=00:10:00 #Tiempo límite (HH:MM:SS)
mpirun --bind-to none nreinasHyb8 $1 $2