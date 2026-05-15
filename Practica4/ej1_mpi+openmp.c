#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <mpi.h>
#include <omp.h>

// Multiplica dos matrices, A y B, de tamaño nxn y almacena el resultado en la matriz C
void matmul(double *A, double *B, double *C, int n);
// Valida el resultado. 0 si ok, sino -1
int validar(int n, double *c, char *fileR);
// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal
double *leerMatriz(double *m, int n, char *fullpath);
// Para calcular tiempo
double dwalltime(void);

// Funcion que realiza el proceso root
void rootProc(char *argv[]);
// Funcion que realiza el proceso worker
void workersProcs();
// Multiplicacion en si de las porciones de matrices asignadas a cada proceso
void multHybrid(void);

// Punteros a las matrices para que trabajen con memoria compartida los threads
double *A, *B, *C;

// Filas de la matriz
int N;

int nProcs;
int idProc;

int nThreads;

int main(int argc, char *argv[])
{
    // Chequeo de parametros
    if ((argc < 6) || ((N = atoi(argv[1])) <= 0) || ((nThreads = atoi(argv[2])) <= 0))
    {
        printf("\nError en los parametros. Usar: %s N <nro threads> <ruta y archivo matriz A> <ruta y archivo matriz B> <ruta y archivo matriz resultado> \n", argv[0]);
        exit(1);
    }

    // TAREA A PARALELIZAR
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &idProc);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcs);

    if (idProc == 0)
    {
        rootProc(argv);
    }
    else
    {
        workersProcs();
    }
    MPI_Finalize();
    return (0);
}

//---------------------------------------------------------------

void rootProc(char *argv[])
{
    int nPart = N * N / nProcs; // Cantidad de filas que hace cada proceso
    // Lee las rutas de los archivos
    char *fileA = argv[3];
    char *fileB = argv[4];
    char *fileR = argv[5];

    // Aloca memoria para las matrices
    A = (double *)malloc(sizeof(double) * N * N);
    B = (double *)malloc(sizeof(double) * N * N);
    C = (double *)malloc(sizeof(double) * N * N);

    // Lee las matrices a y b de archivos. Se almacenan en memoria linealmente tal como se encuentran en archivo
    printf("Leyendo matrices...\n");
    A = leerMatriz(A, N, fileA); // Asumimos ordenada en archivo por filas, en memoria la utilizamos por filas
    B = leerMatriz(B, N, fileB); // Asumimos ordenada en archivo por columnas, en memoria la utilizamos por filas

    // Realiza la multiplicacion
    printf("Multiplicando matrices...\n");

    double timetick = dwalltime();

    // Pasaje de mensajes para compartir las matrices entre los procesos
    MPI_Scatter(A, nPart, MPI_DOUBLE, A, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    MPI_Bcast(B, N * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    multHybrid();

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Gather(C, nPart, MPI_DOUBLE, C, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double workTime = dwalltime() - timetick;

    printf("mm_naive n = %d Tiempo en segundos %f\n", N, workTime);

    // Liberar memoria antes de validar
    free(A);
    free(B);

    // Valida
    printf("Validando...\n");
    if (validar(N, C, fileR) == 0)
        printf("Resultado correcto.\n");
    else
        printf("Error.\n");

    // Libera memoria restante
    free(C);
}

void workersProcs()
{
    // printf("Proceso %d trabajando...\n", idProc);
    int nPart = N * N / nProcs; // Cantidad de filas que hace cada proceso
    A = (double *)malloc(sizeof(double) * nPart);
    B = (double *)malloc(sizeof(double) * N * N);
    C = (double *)malloc(sizeof(double) * nPart);

    MPI_Scatter(A, nPart, MPI_DOUBLE, A, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    MPI_Bcast(B, N * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    /* int ini = 0;
    int fin = nPart / N; */
    multHybrid();

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Gather(C, nPart, MPI_DOUBLE, C, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    free(A);
    free(B);
    free(C);
}

int validar(int n, double *c, char *fileR)
{
    int validacion = 0;
    double *r = (double *)malloc(n * n * sizeof(double));

    leerMatriz(r, n, fileR);

    if (memcmp(r, c, n * n * sizeof(double)) != 0)
    {
        validacion = -1;
    }

    free(r);
    return validacion;
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

void multHybrid()
{
    int nFilasProceso = (N * N / nProcs) / N; // filas locales de este proceso MPI
    double suma;
    int i, j, k;

#pragma omp parallel for private(i, j, k, suma) shared(A, B, C, N) num_threads(nThreads)
    for (i = 0; i < nFilasProceso; i++) // solo las filas locales
    {
        for (j = 0; j < N; j++)
        {
            suma = 0;
            for (k = 0; k < N; k++)
            {
                suma += A[i * N + k] * B[k + N * j];
            }
            C[i * N + j] = suma;
        }
    }
}
