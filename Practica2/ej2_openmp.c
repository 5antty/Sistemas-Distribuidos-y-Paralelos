#include <omp.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

// Lee una matriz desde archivo de a datos consecutivos y la almacena en el mismo orden en memoria principal
double *leerMatriz(double *m, int n, char *fullpath);

// VARIABLES COMPARTIDAS

double *A;
int N;
double max_val = -__DBL_MAX__;
double min_val = __DBL_MAX__;

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
    printf("Leyendo matriz...\n");
    A = leerMatriz(A, N, fileA); // Asumimos ordenada en archivo por filas, en memoria la utilizamos por filas

    int i;
    double suma = 0;
#pragma omp parallel for default(none) private(i) shared(A, N) reduction(max : max_val) reduction(min : min_val) reduction(+ : suma)
    for (i = 0; i < N * N; i++)
    {
        double val = A[i];
        if (val > max_val)
        {
            max_val = val;
        }
        if (val < min_val)
        {
            min_val = val;
        }
        suma += val;
    }
    double promedio = (double)(suma / (N * N));
    printf("Valor maximo de la matriz: %.17f\n", max_val);
    printf("Valor minimo de la matriz: %.17f\n", min_val);
    printf("El promedio de todos los elementos: %.17f\n", promedio);
    free(A);

    return 0;
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