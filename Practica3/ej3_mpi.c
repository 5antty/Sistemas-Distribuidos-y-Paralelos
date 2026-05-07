#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#define NUM_THREADS 4

// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal
double *leerMatriz(double *m, int n, char *fullpath);
// Para calcular tiempo
double dwalltime(void);

// Funcion que realiza el proceso root
void rootProc(int, char *argv[], int nProcs, double *, double *, double *);
// Funcion que realiza el proceso worker
void workersProcs(int, int nProcs, double *, double *, double *);

int N;

int main(int argc, char *argv[])
{
    // Chequeo de parametros
    if ((argc < 3) || ((N = atoi(argv[1])) <= 0))
    {
        printf("\nError en los parametros. Usar: %s N  <ruta y archivo matriz A> <ruta y archivo matriz B> <ruta y archivo matriz resultado> \n", argv[0]);
        exit(1);
    }

    // TAREA A PARALELIZAR
    MPI_Init(&argc, &argv);
    int id, nProcs;
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcs);

    if (id == 0)
    {
        rootProc(id, argv, nProcs, &suma, &min, &max);
    }
    else
    {
        workersProcs(id, nProcs, &suma, &min, &max);
    }
    MPI_Finalize();
    return (0);
}

//---------------------------------------------------------------

void rootProc(int id, char *argv[], int nProcs, double *suma, double *min, double *max)
{

    int nPart = N * N / nProcs; // Cantidad de filas que hace cada proceso
    // Lee las rutas de los archivos
    char *fileA = argv[2];

    // Aloca memoria para las matrices
    double *A;
    A = (double *)malloc(sizeof(double) * N * N);

    // Lee las matrices a de archivos.
    printf("Leyendo matriz...\n");
    A = leerMatriz(A, N, fileA); // Asumimos ordenada en archivo por filas, en memoria la utilizamos por filas
    // Realiza la multiplicacion
    printf("Obteniendo resultados...\n");

    double timetick = dwalltime();

    // Pasaje de mensajes para compartir las matrices entre los procesos
    MPI_Scatter(A, nPart, MPI_DOUBLE, A, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // TRASPONER

    double workTime = dwalltime() - timetick;
    printf("Tiempo en segundos de la ejecucion %f\n", workTime);

    // Liberar memoria antes de validar
    free(A);
}

void workersProcs(int id, int nProcs, double *suma, double *min, double *max)
{

    int nPart = N * N / nProcs; // Carga de trabajo de cada proceso (en este caso filas)
    double *A;
    A = (double *)malloc(sizeof(double) * nPart);

    MPI_Scatter(A, nPart, MPI_DOUBLE, A, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // TRASPONER

    free(A);
}

double *leerMatriz(double *m, int n, char *fullpath)
{
    int i, j;
    double val;
    FILE *archivo;

    archivo = fopen(fullpath, "rb");
    if (!archivo)
    {
        perror("Error al abrir el archivo\n");
        return NULL;
    }

    fread(m, sizeof(double), n * n, archivo);

    fclose(archivo);

    return m;
}

double dwalltime(void)
{
    double sec;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    sec = tv.tv_sec + tv.tv_usec / 1000000.0;
    return sec;
}