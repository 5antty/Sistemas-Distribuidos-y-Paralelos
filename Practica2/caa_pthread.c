#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <bits/pthreadtypes.h>

#define NUM_THREADS 4

// Valida el resultado. 0 si ok, sino -1
int validar(int n, double *c, char *fileR);
// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal
double *leerMatriz(double *m, int n, char *fullpath);
// Para calcular tiempo
double dwalltime(void);

// MULTIPLICACION POR HILOS
void multixthread(int ini, int fin);

// REORDENAMIENTO DE MATRIZ
void reordenarMatriz(int ini, int fin);

void *func(void *arg);

// VARIABLES COMPARTIDAS ENTRE HILOS

int N;
double *A, *C, *Ac;
pthread_barrier_t barrera;

int main(int argc, char *argv[])
{
    // Chequeo de parametros
    if ((argc < 3) || ((N = atoi(argv[1])) <= 0))
    {
        printf("\nError en los parametros. Usar: %s N  <ruta y archivo matriz A> <ruta y archivo matriz B> <ruta y archivo matriz resultado> \n", argv[0]);
        exit(1);
    }

    // Lee las rutas de los archivos
    char *fileA = argv[2];

    // Aloca memoria para las matrices
    A = (double *)malloc(sizeof(double) * N * N);
    C = (double *)malloc(sizeof(double) * N * N);

    // Cambio la matriz A para que este ordenada por columnas.
    Ac = (double *)malloc(sizeof(double) * N * N);

    // Lee las matrices a y b de archivos. Se almacenan en memoria linealmente tal como se encuentran en archivo
    printf("Leyendo matrices...\n");
    A = leerMatriz(A, N, fileA); // Asumimos ordenada en archivo por filas, en memoria la utilizamos por filas

    // CREACION DE HILOS
    pthread_t threads[NUM_THREADS];
    pthread_barrier_init(&barrera, NULL, NUM_THREADS);

    // Realiza la multiplicacion
    printf("Multiplicando matrices...\n");
    double timetick = dwalltime();

    // TAREA A PARALELIZAR
    int ids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
    {
        ids[i] = i;
        pthread_create(&threads[i], NULL, func, (void *)&ids[i]);
    }

    // Espera a que todos los hilos terminen
    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    double workTime = dwalltime() - timetick;

    printf("mm_naive n = %d Tiempo en segundos %f\n", N, workTime);

    // Liberar memoria antes de validar
    free(A);
    free(Ac);

    // Libera memoria restante
    free(C);

    return (0);
}

void *func(void *arg)
{
    // DIVISION DE TRABAJO ENTRE HILOS
    int *id = (int *)arg;
    int ini = *id * (N / NUM_THREADS);
    int fin = ini + (N / NUM_THREADS);

    reordenarMatriz(ini, fin);

    pthread_barrier_wait(&barrera);

    multixthread(ini, fin);
}

void reordenarMatriz(int ini, int fin)
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

void multixthread(int ini, int fin)
{
    int i, j, k;
    double suma;

    for (i = ini; i < fin; i++)
    {
        for (j = 0; j < N; j++)
        {
            suma = 0;
            for (k = 0; k < N; k++)
            {
                suma += A[i * N + k] * Ac[k + N * j];
            }
            C[i * N + j] = suma;
        }
    }
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