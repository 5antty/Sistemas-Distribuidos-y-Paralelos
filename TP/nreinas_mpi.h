#include <mpi.h>
#include "nreinas.c"
void inicializacionMPI(int argc, char *argv[], int *idProc, int *nProcs);
void rootProc(int *SIZE);
void workersProcs();
void finalizacionMPI();