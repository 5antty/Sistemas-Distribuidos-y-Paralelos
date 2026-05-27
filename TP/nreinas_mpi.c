#include "nreinas_mpi.h"
void inicializacionMPI(int argc, char *argv[], int *idProc, int *nProcs)
{
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, idProc);
    MPI_Comm_size(MPI_COMM_WORLD, nProcs);
}

void rootProc(int *SIZE)
{
    int BOARD[MAXSIZE], *BOARDE;
    long int COUNT8 = 0, COUNT4 = 0, COUNT2 = 0;
    long int LCOUNT8 = 0, LCOUNT4 = 0, LCOUNT2 = 0;
    MPI_Bcast(SIZE, 1, MPI_INT, 0, MPI_COMM_WORLD);
    laburar1(*SIZE, BOARD, &LCOUNT8);
}
void workersProcs(int *SIZE)
{
    int BOARD[MAXSIZE], *BOARDE;
    long int LCOUNT8 = 0, LCOUNT4 = 0, LCOUNT2 = 0;

    MPI_Bcast(SIZE, 1, MPI_INT, 0, MPI_COMM_WORLD);
    laburar1(*SIZE, BOARD, &LCOUNT8);
}

void laburar1(int SIZE, int *BOARD, long int *LCOUNT8)
{
    int SIZEE = SIZE - 1;
    int MASK = (1 << SIZE) - 1;
    int BOUND1, BOUND2;
    SIZEE = SIZE - 1;
    // Recién aquí el loop tiene sentido, porque SIZEE ya existe
    for (BOUND1 = 2 + rank; BOUND1 < SIZEE; BOUND1 += size)
    {
        BOARD[1] = bit = 1 << BOUND1;
        Backtrack1(2, (2 | bit) << 1, 1 | bit, bit >> 1, LCOUNT8);
    }
}

void finalizacionMPI()
{
    MPI_Finalize();
}