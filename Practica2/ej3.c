#include <omp.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>

// Valida el resultado. 0 si ok, sino -1
int validar(int n, double *c, char *fileR);
// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal
double *leerMatriz(double *m, int n, char *fullpath);
// Para calcular tiempo
double dwalltime(void);

void trasponerMatriz();
void trasponerMatriz2();

int N;
double *A;

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

    // Lee las matrices a y b de archivos. Se almacenan en memoria linealmente tal como se encuentran en archivo
    printf("Leyendo matrices...\n");
    A = leerMatriz(A, N, fileA); // Asumimos ordenada en archivo por filas, en memoria la utilizamos por filas

    // Realiza la trasposicion 1
    printf("Trasponiendo matriz metodo 1...\n");
    double timetick = dwalltime();
    // TAREA A PARALELIZAR
    trasponerMatriz();
    double workTime = dwalltime() - timetick;
    printf("Trasponer 1 Tiempo en segundos %f\n", workTime);

    // Realiza la trasposicion 2
    printf("Trasponiendo matriz metodo 1...\n");
    timetick = dwalltime();
    // TAREA A PARALELIZAR
    trasponerMatriz2();
    workTime = dwalltime() - timetick;
    printf("Trasponer 2 Tiempo en segundos %f\n", workTime);

    // Liberar memoria antes de validar
    free(A);

    return (0);
}

void trasponerMatriz()
{
    int i, j, tid;
    double temp, timetick;
#pragma omp parallel default(none) private(i, j, temp, timetick, tid) shared(A, N)
    {
        tid = omp_get_thread_num();
        timetick = dwalltime();
#pragma omp for private(i, j, temp) nowait
        for (i = 0; i < N; i++)
        {
            for (j = i + 1; j < N; j++)
            {
                temp = A[i * N + j];
                A[i * N + j] = A[j * N + i];
                A[j * N + i] = temp;
            }
        }
        printf("Tiempo para el thread %d: %f segs\n", tid, dwalltime() - timetick);
    }
}

void trasponerMatriz2()
{
    int i, j, tid;
    double temp, timetick;
#pragma omp parallel default(none) private(i, j, temp, timetick, tid) shared(A, N)
    {
        tid = omp_get_thread_num();
        timetick = dwalltime();
#pragma omp for private(i, j, temp) schedule(dynamic, 1) nowait
        for (i = 0; i < N; i++)
        {
            for (j = i + 1; j < N; j++)
            {
                temp = A[i * N + j];
                A[i * N + j] = A[j * N + i];
                A[j * N + i] = temp;
            }
        }
        printf("Tiempo para el thread %d: %f segs\n", tid, dwalltime() - timetick);
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