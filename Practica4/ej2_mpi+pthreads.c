#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <omp.h>

// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal
double *leerMatriz(double *m, int n, char *fullpath);
// Para calcular tiempo
double dwalltime(void);

// Funcion que realiza el proceso root
void rootProc(char *argv[], double *, double *, double *);
// Funcion que realiza el proceso worker
void workersProcs(double *, double *, double *);
// Recorrido de las porciones de matrices
void recorrido(double *suma, double *min, double *max);

// Punteros a las matrices para que trabajen con memoria compartida los threads
double *A;

// Filas de la matriz
int N;

int nProcs;
int idProc;

int nThreads;

int main(int argc, char *argv[])
{
    // Chequeo de parametros
    if ((argc < 4) || ((N = atoi(argv[1])) <= 0) || ((nThreads = atoi(argv[2])) <= 0))
    {
        printf("\nError en los parametros. Usar: %s N <threads> <ruta y archivo matriz A>  \n", argv[0]);
        exit(1);
    }

    // TAREA A PARALELIZAR
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &idProc);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcs);

    double suma = 0;
    double max = -__DBL_MAX__;
    double min = __DBL_MAX__;

    if (idProc == 0)
    {
        rootProc(argv, &suma, &min, &max);
    }
    else
    {
        workersProcs(&suma, &min, &max);
    }
    MPI_Finalize();
    return (0);
}

//---------------------------------------------------------------

void rootProc(char *argv[], double *suma, double *min, double *max)
{

    int nPart = N * N / nProcs; // Cantidad de filas que hace cada proceso
    // Lee las rutas de los archivos
    char *fileA = argv[3];

    // Aloca memoria para las matrices
    // double *A;
    A = (double *)malloc(sizeof(double) * N * N);

    // Lee las matrices a de archivos.
    printf("Leyendo matriz...\n");
    A = leerMatriz(A, N, fileA); // Asumimos ordenada en archivo por filas, en memoria la utilizamos por filas
    // Realiza la multiplicacion
    printf("Obteniendo resultados...\n");

    double timetick = dwalltime();

    // Pasaje de mensajes para compartir las matrices entre los procesos
    MPI_Scatter(A, nPart, MPI_DOUBLE, A, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    recorrido(suma, min, max);

    double workTime = dwalltime() - timetick;
    printf("Tiempo en segundos de la ejecucion %f\n", workTime);

    printf("Promedio de los elementos de la matriz A: %f\n", (*(suma) / (N * N)));
    printf("Valor maximo de la matriz A: %f\n", *(max));
    printf("Valor minimo de la matriz A: %f\n", *(min));

    // Liberar memoria antes de validar
    free(A);
}

void workersProcs(double *suma, double *min, double *max)
{

    int nPart = N * N / nProcs; // Carga de trabajo de cada proceso (en este caso filas)
    // double *A;
    A = (double *)malloc(sizeof(double) * nPart);

    MPI_Scatter(A, nPart, MPI_DOUBLE, A, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    recorrido(suma, min, max);

    free(A);
}

void recorrido(double *suma, double *min, double *max)
{
    int nPart = N * N / nProcs;
    double loc_suma = 0;
    double loc_max = -__DBL_MAX__;
    double loc_min = __DBL_MAX__;
    double val;
    int i;
#pragma omp parallel for default(none) private(i, val) shared(A, nPart) reduction(max : loc_max) reduction(min : loc_min) reduction(+ : loc_suma) num_threads(nThreads)
    for (i = 0; i < nPart; i++)
    {
        val = A[i];
        if (val > loc_max)
        {
            loc_max = val;
        }
        if (val < loc_min)
        {
            loc_min = val;
        }
        loc_suma += val;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Reduce(&loc_suma, suma, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    MPI_Reduce(&loc_max, max, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    MPI_Reduce(&loc_min, min, 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);

    // printf("Proceso %d suma %f\n", id, loc_suma);
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