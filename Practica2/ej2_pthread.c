#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <bits/pthreadtypes.h>

#define NUM_THREADS 4

// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal
double *leerMatriz(double *m, int n, char *fullpath);
// Para calcular tiempo
double dwalltime(void);

// RECORRIDO DE MATRIZ POR HILOS
void recorrer(int ini, int fin, double *suma, double *min_val, double *max_val);

void *func(void *arg);

// VARIABLES COMPARTIDAS

double *A;
int N;
double max_val = -__DBL_MAX__;
double min_val = __DBL_MAX__;
double suma = 0;

pthread_mutex_t mutex_sum;
pthread_mutex_t mutex_min;
pthread_mutex_t mutex_max;

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

    pthread_t threads[NUM_THREADS];
    pthread_mutex_init(&mutex_sum, NULL);
    pthread_mutex_init(&mutex_min, NULL);
    pthread_mutex_init(&mutex_max, NULL);

    // Lee las matrices a y b de archivos. Se almacenan en memoria linealmente tal como se encuentran en archivo
    printf("Leyendo matriz...\n");
    A = leerMatriz(A, N, fileA); // Asumimos ordenada en archivo por filas, en memoria la utilizamos por filas

    int ids[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++)
    {
        ids[i] = i;
        pthread_create(&threads[i], NULL, func, (void *)&ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        pthread_join(threads[i], NULL);
    }

    double promedio = (double)(suma / (N * N));
    printf("Valor maximo de la matriz: %.17f\n", max_val);
    printf("Valor minimo de la matriz: %.17f\n", min_val);
    printf("El promedio de todos los elementos: %.17f\n", promedio);
    free(A);

    return 0;
}

void *func(void *arg)
{
    // DIVISION DE TRABAJO ENTRE HILOS
    int *id = (int *)arg;
    // Distribucion de carga entre hilos por filas. NO es del todo correcto
    /*int ini = *id * (N / NUM_THREADS);
    int fin = ini + (N / NUM_THREADS);
    */

    // Distribucion de carga entre hilos por celdas. Es mas balanceada
    int ini = *id * (N * N / NUM_THREADS);
    int fin = ini + (N * N / NUM_THREADS);
    double suma_thread, min_val_thread, max_val_thread;

    recorrer(ini, fin, &suma_thread, &min_val_thread, &max_val_thread);

    pthread_mutex_lock(&mutex_sum);
    suma += suma_thread;
    pthread_mutex_unlock(&mutex_sum);
    pthread_mutex_lock(&mutex_min);
    if (min_val_thread < min_val)
    {
        min_val = min_val_thread;
    }
    pthread_mutex_unlock(&mutex_min);
    pthread_mutex_lock(&mutex_max);
    if (max_val_thread > max_val)
    {
        max_val = max_val_thread;
    }
    pthread_mutex_unlock(&mutex_max);
}

void recorrer(int ini, int fin, double *suma, double *min_val, double *max_val)
{
    int i, j;
    double loc_sum = 0;
    double loc_min = __DBL_MAX__;
    double loc_max = -__DBL_MAX__;
    double val;

    for (i = ini; i < fin; i++)
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
        loc_sum += val;
    }
    *suma = loc_sum;
    *min_val = loc_min;
    *max_val = loc_max;
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