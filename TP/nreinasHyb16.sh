#!/bin/bash
#SBATCH -N 2
#SBATCH --exclusive
#SBATCH --partition=Blade
#SBATCH --tasks-per-node=1
#SBATCH -o Salidas16/output.txt
#SBATCH -e Salidas16/errors.txt
#SBATCH --time=00:10:00 #Tiempo límite (HH:MM:SS)
mpirun --bind-to none nreinasHyb $1 $2