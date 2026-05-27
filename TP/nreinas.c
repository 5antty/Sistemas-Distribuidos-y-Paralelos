/**************************************************************************/
/* N-Queens Solutions  ver3.1               takaken July/2003             */
/**************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include "nreinas_mpi.h"

/* Time in seconds from some point in the past */
double dwalltime();

#define MAXSIZE 24
#define MINSIZE 2

//
// VARIABLES LOCALES A CADA PROCESO
//

// SIZE es el tamaño del tablero,
// y SIZEE es el tamaño del tablero - 1
int SIZE, SIZEE;
// BOARD es el tablero (de mayor tamaño), BOARDE es un puntero
// al ultimo elemento del tablero segun SIZEE
int BOARD[MAXSIZE], *BOARDE;

// BOARD1 y BOARD2 son punteros a elementos del
// tablero que se usan para comparar
int *BOARD1, *BOARD2;
// TOPBIT es el bit mas significativo del tablero,
// que se usa para colocar la reina en la primera fila,
// y luego ir desplazando ese bit para colocar las reinas
int TOPBIT;
// Sirve para ignorar los bits que queden
// fuera del tamaño del tablero.
int MASK;
int SIDEMASK, LASTMASK, ENDBIT;
// BOUND1 es el limite superior para colocar la primera reina, y BOUND2 el limite inferior
int BOUND1, BOUND2;

long int COUNT8, COUNT4, COUNT2;
long int TOTAL, UNIQUE;

/**********************************************/
/* Display the Board Image                    */
/**********************************************/
void Display(void)
{
    int y, bit;

    printf("N= %d\n", SIZE);
    for (y = 0; y < SIZE; y++)
    {
        for (bit = TOPBIT; bit; bit >>= 1)
            printf("%s ", (BOARD[y] & bit) ? "Q" : "-");
        printf("\n");
    }
    printf("\n");
}
/**********************************************/
/* Check Unique Solutions                     */
/**********************************************/
void Check(void)
{
    int *own, *you, bit, ptn;

    /* 90-degree rotation */
    if (*BOARD2 == 1)
    {
        for (ptn = 2, own = BOARD + 1; own <= BOARDE; own++, ptn <<= 1)
        {
            bit = 1;
            for (you = BOARDE; *you != ptn && *own >= bit; you--)
                bit <<= 1;
            if (*own > bit)
                return;
            if (*own < bit)
                break;
        }
        if (own > BOARDE)
        {
            COUNT2++;
            // Display();
            return;
        }
    }

    /* 180-degree rotation */
    if (*BOARDE == ENDBIT)
    {
        for (you = BOARDE - 1, own = BOARD + 1; own <= BOARDE; own++, you--)
        {
            bit = 1;
            for (ptn = TOPBIT; ptn != *you && *own >= bit; ptn >>= 1)
                bit <<= 1;
            if (*own > bit)
                return;
            if (*own < bit)
                break;
        }
        if (own > BOARDE)
        {
            COUNT4++;
            // Display();
            return;
        }
    }

    /* 270-degree rotation */
    if (*BOARD1 == TOPBIT)
    {
        for (ptn = TOPBIT >> 1, own = BOARD + 1; own <= BOARDE; own++, ptn >>= 1)
        {
            bit = 1;
            for (you = BOARD; *you != ptn && *own >= bit; you++)
                bit <<= 1;
            if (*own > bit)
                return;
            if (*own < bit)
                break;
        }
    }
    COUNT8++;
    // Display();
}
/**********************************************/
/* First queen is inside                      */
/**********************************************/
void Backtrack2(int y, int left, int down, int right)
{
    int bitmap, bit;

    bitmap = MASK & ~(left | down | right);
    if (y == SIZEE)
    {
        if (bitmap)
        {
            if (!(bitmap & LASTMASK))
            {
                BOARD[y] = bitmap;
                Check();
            }
        }
    }
    else
    {
        if (y < BOUND1)
        {
            bitmap |= SIDEMASK;
            bitmap ^= SIDEMASK;
        }
        else if (y == BOUND2)
        {
            if (!(down & SIDEMASK))
                return;
            if ((down & SIDEMASK) != SIDEMASK)
                bitmap &= SIDEMASK;
        }
        while (bitmap)
        {
            bitmap ^= BOARD[y] = bit = -bitmap & bitmap;
            Backtrack2(y + 1, (left | bit) << 1, down | bit, (right | bit) >> 1);
        }
    }
}
/**********************************************/
/* First queen is in the corner               */
/**********************************************/
void Backtrack1(int y, int left, int down, int right, long int *LCOUNT8)
{
    int bitmap, bit;

    bitmap = MASK & ~(left | down | right);
    if (y == SIZEE)
    {
        if (bitmap)
        {
            BOARD[y] = bitmap;
            (*LCOUNT8)++;
            // Display();
        }
    }
    else
    {
        if (y < BOUND1)
        {
            bitmap |= 2;
            bitmap ^= 2;
        }
        while (bitmap)
        {
            bitmap ^= BOARD[y] = bit = -bitmap & bitmap;
            Backtrack1(y + 1, (left | bit) << 1, down | bit, (right | bit) >> 1);
        }
    }
}
/**********************************************/
/* Search of N-Queens                         */
/**********************************************/
void NQueens(void)
{
    int bit, cant;

    /* Initialize */
    COUNT8 = COUNT4 = COUNT2 = 0;
    SIZEE = SIZE - 1;
    // BOARDE apunta al ultimo elemento del tablero segun SIZEE
    BOARDE = &BOARD[SIZEE];

    TOPBIT = 1 << SIZEE;
    printf("topbit: %d\n", TOPBIT);
    MASK = (1 << SIZE) - 1;

    /* 0:000000001 */
    /* 1:011111100 */
    // Primera reina en la esquina, entonces el primer bit
    // (el de la derecha) del tablero se pone en 1, y el resto en 0
    BOARD[0] = 1;

    for (BOUND1 = 2; BOUND1 < SIZEE; BOUND1++)
    {
        BOARD[1] = bit = 1 << BOUND1;
        Backtrack1(2, (2 | bit) << 1, 1 | bit, bit >> 1);
    }
    /* 0:000001110 */
    SIDEMASK = LASTMASK = TOPBIT | 1;
    ENDBIT = TOPBIT >> 1;
    for (BOUND1 = 1, BOUND2 = SIZE - 2; BOUND1 < BOUND2; BOUND1++, BOUND2--)
    {
        BOARD1 = &BOARD[BOUND1];
        BOARD2 = &BOARD[BOUND2];
        BOARD[0] = bit = 1 << BOUND1;
        Backtrack2(1, bit << 1, bit, bit >> 1);
        LASTMASK |= LASTMASK >> 1 | LASTMASK << 1;
        ENDBIT >>= 1;
    }

    /* Unique and Total Solutions */
    UNIQUE = COUNT8 + COUNT4 + COUNT2;
    TOTAL = COUNT8 * 8 + COUNT4 * 4 + COUNT2 * 2;
}

/**********************************************/
/* N-Queens Solutions MAIN                    */
/**********************************************/
int main(int argC, char *argV[])
{
    double tIni, tFin;
    int idProc, nProcs;
    SIZE = atoi(argV[1]);
    inicializacionMPI(argC, argV, &idProc, &nProcs);
    tIni = dwalltime();
    NQueens();
    tFin = dwalltime();

    printf("Número de resultados: %lu -  Tiempo Total: %f segundos \n", TOTAL, tFin - tIni);
    finalizacionMPI();
    return 0;
}
#include <sys/time.h>

double dwalltime()
{
    double sec;
    struct timeval tv;

    gettimeofday(&tv, NULL);
    sec = tv.tv_sec + tv.tv_usec / 1000000.0;
    return sec;
}

/*
    Ejecutar en 2 nodos, cada nodo tiene 2 intel xeon
    cada intel xeon tiene 4 cores
    Un nodo tiene 8 cores, entonces en total
    tengo 16 cores para ejecutar el programa

    compilo en frontend y luego ejecuto con sbatch el binario compilado

    EN principio tengo que dividir el trabajo entre los procesos
    como todos deberian saber el estado del tablero, podria hacer
    un broadcast del estado del tablero a todos los procesos,
    y luego cada proceso se encarga de buscar soluciones a partir
    de ese estado del tablero, y al finalizar cada proceso hace
    un reduce para sumar la cantidad de soluciones encontradas por cada proceso,
    y asi obtener el resultado final
    */

// usar semaforos para controlar el acceso a la variable
// que cuenta las soluciones encontradas por cada proceso,
// para evitar condiciones de carrera