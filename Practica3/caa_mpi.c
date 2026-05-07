#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <mpi.h>

// Multiplica dos matrices, A y B, de tamaño nxn y almacena el resultado en la matriz C
void matmul(double *A, double *B, double *C, int n);
// Valida el resultado. 0 si ok, sino -1
int validar(int n, double *c, char *fileR);
// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal
double *leerMatriz(double *m, int n, char *fullpath);
// Para calcular tiempo
double dwalltime(void);

// Funcion que realiza el proceso root
void rootProc(int id, char *argv[], int nProcs);
// Funcion que realiza el proceso worker
void workersProcs(int, int nProcs);
// Multiplicacion en si de las porciones de matrices asignadas a cada proceso
void mult(int, double *A, double *B, double *C, int ini, int fin);
// Reordenamiento de matriz A para que este ordenada por columnas
void reordenarMatriz(int ini, int fin, double *A, double *Ac);

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
        rootProc(id, argv, nProcs);
    }
    else
    {
        workersProcs(id, nProcs);
    }
    MPI_Finalize();
    return (0);
}

//---------------------------------------------------------------

void rootProc(int id, char *argv[], int nProcs)
{

    int nPart = N * N / nProcs; // Cantidad de filas que hace cada proceso
    // Lee las rutas de los archivos
    char *fileA = argv[2];

    // Aloca memoria para las matrices
    double *A, *Ac, *C;
    A = (double *)malloc(sizeof(double) * N * N);
    Ac = (double *)malloc(sizeof(double) * N * N);
    C = (double *)malloc(sizeof(double) * N * N);

    // Lee las matrices a de archivos.
    printf("Leyendo matrices...\n");
    A = leerMatriz(A, N, fileA); // Asumimos ordenada en archivo por filas, en memoria la utilizamos por filas
    // Realiza la multiplicacion
    printf("Multiplicando matrices...\n");

    double timetick = dwalltime();

    // Pasaje de mensajes para compartir las matrices entre los procesos
    MPI_Scatter(A, nPart, MPI_DOUBLE, A, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    reordenarMatriz(0, nPart / N, A, Ac);

    MPI_Gather(Ac, nPart, MPI_DOUBLE, Ac, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD); // Asegura que todos los procesos hayan terminado de reordenar antes de continuar

    MPI_Bcast(Ac, N * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    mult(id, A, Ac, C, 0, nPart / N);

    MPI_Gather(C, nPart, MPI_DOUBLE, C, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double workTime = dwalltime() - timetick;

    printf("mm_naive n = %d Tiempo en segundos %f\n", N, workTime);

    // Liberar memoria antes de validar
    free(A);
    free(Ac);

    // Libera memoria restante
    free(C);
}

void workersProcs(int id, int nProcs)
{

    int nPart = N * N / nProcs; // Cantidad de filas que hace cada proceso
    double *A, *Ac, *C;
    A = (double *)malloc(sizeof(double) * nPart);
    Ac = (double *)malloc(sizeof(double) * N * N);
    C = (double *)malloc(sizeof(double) * nPart);

    int ini = 0;
    int fin = nPart / N;

    MPI_Scatter(A, nPart, MPI_DOUBLE, A, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    reordenarMatriz(ini, fin, A, Ac);

    MPI_Gather(Ac, nPart, MPI_DOUBLE, Ac, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    MPI_Barrier(MPI_COMM_WORLD); // Asegura que todos los procesos hayan terminado de reordenar antes de continuar

    MPI_Bcast(Ac, N * N, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    mult(id, A, Ac, C, ini, fin);

    MPI_Gather(C, nPart, MPI_DOUBLE, C, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    free(A);
    free(Ac);
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

void mult(int id, double *A, double *Ac, double *C, int ini, int fin)
{
    double suma;
    for (int i = ini; i < fin; i++)
    {
        for (int j = 0; j < N; j++)
        {
            suma = 0;
            for (int k = 0; k < N; k++)
            {
                suma += A[i * N + k] * Ac[k + N * j];
                // printf("Proceso %d multiplicando A[%d][%d] = %f * B[%d][%d] = %f\n", id, i, k, A[i * N + k], k, j, B[k + N * j]);
            }
            C[i * N + j] = suma;
        }
    }
}

void reordenarMatriz(int ini, int fin, double *A, double *Ac)
{
    int i, j;
    for (i = ini; i < fin; i++)
    {
        for (j = 0; j < N; j++)
        {
            Ac[i + j * N] = A[i * N + j];
        }
    }
}