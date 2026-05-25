#include <mpi.h>

void divisionProcesos()
{
    MPI_Broadcast(&SIZE, 1, MPI_INT, 0, MPI_COMM_WORLD);
    // MPI_Barrier(MPI_COMM_WORLD);
}