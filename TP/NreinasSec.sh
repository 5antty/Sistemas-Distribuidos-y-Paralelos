#!/bin/bash
#SBATCH -N 1
#SBATCH --exclusive
#SBATCH --partition=Blade
#SBATCH -o Salidas/output.txt
#SBATCH -e Salidas/errors.txt
#SBATCH --time=00:10:00 #Tiempo límite (HH:MM:SS)
./nreinas $1