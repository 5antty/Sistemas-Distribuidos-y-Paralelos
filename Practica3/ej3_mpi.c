#include <mpi.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

#define NUM_THREADS 4

// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal
double *leerMatriz(double *m, int n, char *fullpath);
// Para calcular tiempo
double dwalltime(void);

// Funcion que realiza el proceso root
void rootProc(int, char *argv[], int nProcs);
// Funcion que realiza el proceso worker
void workersProcs(int, int nProcs);
// Funcion que realiza la trasposicion de las matrices parciales
void trasponer(double *A, int nPart);

void showMatrix(double *A, int n);

int N = 4;

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
    double *A;
    A = (double *)malloc(sizeof(double) * N * N);

    // Lee las matrices a de archivos.
    printf("Leyendo matriz...\n");
    // A = leerMatriz(A, N, fileA); // Asumimos ordenada en archivo por filas, en memoria la utilizamos por filas

    double valores_A[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16};
    // Copiamos los datos a la memoria que reservaste
    memcpy(A, valores_A, sizeof(double) * N * N);

    // Realiza la multiplicacion
    printf("Obteniendo resultados...\n");
    showMatrix(A, N * N);

    double timetick = dwalltime();

    // Pasaje de mensajes para compartir las matrices entre los procesos
    MPI_Scatter(A, nPart, MPI_DOUBLE, A, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // TRASPONER
    trasponer(A, nPart);

    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Gather(A, nPart, MPI_DOUBLE, A, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    double workTime = dwalltime() - timetick;
    printf("Tiempo en segundos de la ejecucion %f\n", workTime);

    showMatrix(A, N * N);

    // Liberar memoria antes de validar
    free(A);
}

// HASTA AHORA TRASPONGO EN LAS FILAS, TENGO QUE ARMAR LOS BLOQUES?

void workersProcs(int id, int nProcs)
{

    int nPart = N * N / nProcs; // Carga de trabajo de cada proceso (en este caso filas)
    double *A;
    A = (double *)malloc(sizeof(double) * nPart);

    MPI_Scatter(A, nPart, MPI_DOUBLE, A, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    printf("Proceso %d recibió la parte de la matriz:\n", id);
    showMatrix(A, nPart); // Mostrar la parte de la matriz que le corresponde a cada proceso

    // TRASPONER
    trasponer(A, nPart);

    MPI_Barrier(MPI_COMM_WORLD);
    // Asegurarse de que todos los procesos hayan terminado la trasposición antes de recolectar los resultados

    MPI_Gather(A, nPart, MPI_DOUBLE, A, nPart, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    free(A);
}

void trasponer(double *A, int nPart)
{
    double temp;
    double aux = sqrt(nPart);
    int aux2 = (int)aux;
    for (int i = 0; i < aux2; i++)
    {
        for (int j = i + 1; j < aux2; j++)
        {
            temp = A[i * aux2 + j];
            A[i * aux2 + j] = A[j * aux2 + i];
            A[j * aux2 + i] = temp;
        }
    }
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

void showMatrix(double *A, int n)
{
    double aux = sqrt(n);
    int aux2 = (int)aux;
    for (int i = 0; i < aux2; i++)
    {
        for (int j = 0; j < aux2; j++)
        {
            printf("%f ", A[i * aux2 + j]);
        }
        printf("\n");
    }
}

// Hacer scatter de filas y columnas, cada proceso traspone una fila y una columna, inevitablemente va a haber desbalance de carga.