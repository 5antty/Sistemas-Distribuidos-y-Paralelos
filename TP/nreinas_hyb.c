/**************************************************************************/
/* N-Queens Solutions  ver3.1 + MPI + Pthreads                            */
/**************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <mpi.h>
#include <sys/time.h>

double dwalltime();

#define MAXSIZE 24
#define MINSIZE 2

/* ------------------------------------------------------------------ */
/* Tipo de tarea: indica qué backtrack ejecutar                         */
/* ------------------------------------------------------------------ */
typedef enum
{
    TASK_BT1,
    TASK_BT2
} TaskType;

/* ------------------------------------------------------------------ */
/* Descriptor de una tarea individual                                   */
/*   - Para TASK_BT1 sólo importa bound1                               */
/*   - Para TASK_BT2 importan bound1 y bound2                          */
/* ------------------------------------------------------------------ */
typedef struct
{
    TaskType type;
    int bound1;
    int bound2; /* usado sólo en TASK_BT2 */
} Task;

/* ------------------------------------------------------------------ */
/* Estado privado de cada thread                                        */
/*   Toda variable que se modifica durante el backtracking vive aquí.  */
/*   Las variables de solo lectura (size, mask, etc.) se leen de los   */
/*   globales sin problema porque no se escriben durante la búsqueda.  */
/* ------------------------------------------------------------------ */
typedef struct
{
    /* --- tarea asignada --- */
    Task *tasks;    /* puntero al array de tareas del proceso */
    int task_start; /* índice de la primera tarea de este thread */
    int task_end;   /* índice exclusivo de la última tarea       */

    /* --- estado privado del tablero --- */
    int board[MAXSIZE];
    int *boarde; /* &board[sizee]  */
    int *board1; /* &board[bound1] — sólo BT2 */
    int *board2; /* &board[bound2] — sólo BT2 */

    /* --- parámetros de la búsqueda (solo lectura, copiados por comodidad) --- */
    int size, sizee, mask, topbit;

    /* --- contadores propios (sin mutex, se suman después del join) --- */
    long int count8, count4, count2;
} ThreadArgs;

/* ------------------------------------------------------------------ */
/* Globales de solo lectura compartidas entre threads                   */
/* ------------------------------------------------------------------ */
int G_SIZE, G_SIZEE, G_MASK, G_TOPBIT;
long int GLOBAL_COUNT8 = 0, GLOBAL_COUNT4 = 0, GLOBAL_COUNT2 = 0;
long int TOTAL, UNIQUE;
int mpi_rank, mpi_size;
int NUM_THREADS; /* leído de argv[2] */

// Variables donde se alocan las tareas y los argumentos de los threads,
// las hago aca para poder alocar y liberarlos fuera del tiempo de computo
Task *tasks_bt1;
Task *tasks_bt2;
ThreadArgs *args;
pthread_t *tids;
int ntasks_bt1;
int ntasks_bt2;

/* ==================================================================
   BACKTRACK 1  — versión thread-safe
   Recibe el estado completo por puntero en lugar de usar globales.
   BOUND1 se pasa directo porque es un parámetro de la iteración.
   ================================================================== */

static void
Backtrack1_t(ThreadArgs *a, int y, int left, int down, int right,
             int bound1)
{
    int bitmap, bit;

    bitmap = a->mask & ~(left | down | right);

    if (y == a->sizee)
    {
        if (bitmap)
        {
            a->board[y] = bitmap;
            a->count8++;
        }
    }
    else
    {
        /* Excluir columna 1 (bit=2) mientras y < bound1 */
        if (y < bound1)
        {
            bitmap |= 2;
            bitmap ^= 2;
        }
        while (bitmap)
        {
            bitmap ^= a->board[y] = bit = -bitmap & bitmap;
            Backtrack1_t(a, y + 1,
                         (left | bit) << 1,
                         down | bit,
                         (right | bit) >> 1,
                         bound1);
        }
    }
}

/* ==================================================================
   CHECK  — versión thread-safe
   Usa punteros del ThreadArgs en lugar de globales BOARD, BOARD1, etc.
   ================================================================== */
static void Check_t(ThreadArgs *a, int lastmask, int endbit)
{
    int *own, *you, bit, ptn;

    /* 90-degree rotation */
    if (*a->board2 == 1)
    {
        for (ptn = 2, own = a->board + 1; own <= a->boarde; own++, ptn <<= 1)
        {
            bit = 1;
            for (you = a->boarde; *you != ptn && *own >= bit; you--)
                bit <<= 1;
            if (*own > bit)
                return;
            if (*own < bit)
                break;
        }
        if (own > a->boarde)
        {
            a->count2++;
            return;
        }
    }

    /* 180-degree rotation */
    if (*a->boarde == endbit)
    {
        for (you = a->boarde - 1, own = a->board + 1; own <= a->boarde; own++, you--)
        {
            bit = 1;
            for (ptn = a->topbit; ptn != *you && *own >= bit; ptn >>= 1)
                bit <<= 1;
            if (*own > bit)
                return;
            if (*own < bit)
                break;
        }
        if (own > a->boarde)
        {
            a->count4++;
            return;
        }
    }

    /* 270-degree rotation */
    if (*a->board1 == a->topbit)
    {
        for (ptn = a->topbit >> 1, own = a->board + 1; own <= a->boarde; own++, ptn >>= 1)
        {
            bit = 1;
            for (you = a->board; *you != ptn && *own >= bit; you++)
                bit <<= 1;
            if (*own > bit)
                return;
            if (*own < bit)
                break;
        }
    }
    a->count8++;
}

/* ==================================================================
   BACKTRACK 2  — versión thread-safe
   sidemask, lastmask, endbit y bound1/bound2 se pasan como parámetros
   porque cambian para cada tarea.
   ================================================================== */
static void Backtrack2_t(ThreadArgs *a, int y, int left, int down, int right,
                         int bound1, int bound2,
                         int sidemask, int lastmask, int endbit)
{
    int bitmap, bit;

    bitmap = a->mask & ~(left | down | right);

    if (y == a->sizee)
    {
        if (bitmap)
        {
            if (!(bitmap & lastmask))
            {
                a->board[y] = bitmap;
                Check_t(a, lastmask, endbit);
            }
        }
    }
    else
    {
        if (y < bound1)
        {
            bitmap |= sidemask;
            bitmap ^= sidemask;
        }
        else if (y == bound2)
        {
            if (!(down & sidemask))
                return;
            if ((down & sidemask) != sidemask)
                bitmap &= sidemask;
        }
        while (bitmap)
        {
            bitmap ^= a->board[y] = bit = -bitmap & bitmap;
            Backtrack2_t(a, y + 1,
                         (left | bit) << 1,
                         down | bit,
                         (right | bit) >> 1,
                         bound1, bound2,
                         sidemask, lastmask, endbit);
        }
    }
}

/* ==================================================================
   FUNCIÓN DE THREAD
   Cada thread recorre el rango [task_start, task_end) de tareas
   que le fue asignado y ejecuta el backtrack correspondiente.
   ================================================================== */
static void *thread_worker(void *arg)
{
    ThreadArgs *a = (ThreadArgs *)arg;

    /* Inicializar estado privado */
    a->count8 = a->count4 = a->count2 = 0;
    a->boarde = &a->board[a->sizee];

    int sidemask_base = a->topbit | 1; /* usado en BT2 */

    for (int i = a->task_start; i < a->task_end; i++)
    {
        Task *t = &a->tasks[i];

        if (t->type == TASK_BT1)
        {
            /* --- Loop 1: reina en la esquina --- */
            int bound1 = t->bound1;
            int bit = 1 << bound1;
            a->board[0] = 1;
            a->board[1] = bit;
            Backtrack1_t(a, 2,
                         (2 | bit) << 1,
                         1 | bit,
                         bit >> 1,
                         bound1);
        }
        else /* TASK_BT2 */
        {
            /* --- Loop 2: reina en el interior --- */
            int bound1 = t->bound1;
            int bound2 = t->bound2;

            /* Calcular lastmask y endbit para este bound1 específico */
            int lastmask = sidemask_base;
            int endbit = a->topbit >> 1;
            for (int j = 0; j < bound1 - 1; j++)
            {
                lastmask |= lastmask >> 1 | lastmask << 1;
                endbit >>= 1;
            }

            /* board1 y board2 apuntan al tablero PRIVADO del thread */
            a->board1 = &a->board[bound1];
            a->board2 = &a->board[bound2];

            int bit = 1 << bound1;
            a->board[0] = bit;
            Backtrack2_t(a, 1,
                         bit << 1, bit, bit >> 1,
                         bound1, bound2,
                         sidemask_base, lastmask, endbit);
        }
    }

    return NULL;
}

/* ==================================================================
   NQueens con Pthreads
   1. Construye la lista de tareas del proceso MPI actual
   2. La reparte cíclicamente entre NUM_THREADS threads
   3. Lanza los threads, espera con join y acumula contadores
   ================================================================== */
void inicializacionEstructuras()
{
    /* ------------------------------------------------------------------
       PASO 1 — Construir lista de tareas del Loop 1 (BT1)
       El loop original era: for(BOUND1 = 2+rank; BOUND1 < SIZEE; BOUND1 += size)
       Aquí generamos exactamente esos valores como tareas individuales.
       ------------------------------------------------------------------ */
    int sizee = G_SIZE - 1;
    int topbit = 1 << sizee;
    int mask = (1 << G_SIZE) - 1;

    int max_tasks_bt1 = G_SIZE; /* cota superior segura */
    tasks_bt1 = malloc(max_tasks_bt1 * sizeof(Task));
    ntasks_bt1 = 0;

    for (int b1 = 2 + mpi_rank; b1 < sizee; b1 += mpi_size)
    {
        tasks_bt1[ntasks_bt1].type = TASK_BT1;
        tasks_bt1[ntasks_bt1].bound1 = b1;
        tasks_bt1[ntasks_bt1].bound2 = 0; /* no usado */
        ntasks_bt1++;
    }

    /* ------------------------------------------------------------------
       PASO 2 — Construir lista de tareas del Loop 2 (BT2)
       El loop original era:
         for(BOUND1=1+rank, BOUND2=SIZE-2-rank; BOUND1<BOUND2; BOUND1+=size, BOUND2-=size)
       Cada iteración es un par (bound1, bound2) → una tarea.
       ------------------------------------------------------------------ */
    int max_tasks_bt2 = G_SIZE;
    tasks_bt2 = malloc(max_tasks_bt2 * sizeof(Task));
    ntasks_bt2 = 0;

    for (int b1 = 1 + mpi_rank, b2 = G_SIZE - 2 - mpi_rank;
         b1 < b2;
         b1 += mpi_size, b2 -= mpi_size)
    {
        tasks_bt2[ntasks_bt2].type = TASK_BT2;
        tasks_bt2[ntasks_bt2].bound1 = b1;
        tasks_bt2[ntasks_bt2].bound2 = b2;
        ntasks_bt2++;
    }

    /* ------------------------------------------------------------------
       PASO 3 — Alocar estructuras de threads y asignar estado de solo lectura
       ------------------------------------------------------------------ */
    args = malloc(NUM_THREADS * sizeof(ThreadArgs));
    tids = malloc(NUM_THREADS * sizeof(pthread_t));

    for (int t = 0; t < NUM_THREADS; t++)
    {
        args[t].size = G_SIZE;
        args[t].sizee = sizee;
        args[t].mask = mask;
        args[t].topbit = topbit;
    }
}
void NQueens(void)
{
    int sizee = G_SIZE - 1;
    int topbit = 1 << sizee;
    int mask = (1 << G_SIZE) - 1;

    /* ------------------------------------------------------------------
       PASO 4 — Lanzar threads para el Loop 1 (BT1)

       Distribución cíclica: si hay 5 tareas y 3 threads:
         Thread 0 → tareas 0, 3
         Thread 1 → tareas 1, 4
         Thread 2 → tareas 2
       Esto se logra dando a cada thread el array completo de tareas
       pero con stride = NUM_THREADS usando task_start y step implícito.

       Alternativa más simple (la que usamos): reparto por bloques
       contiguos, que es más cache-friendly:
         Thread 0 → [0, chunk)
         Thread 1 → [chunk, 2*chunk)
         ...
       ------------------------------------------------------------------ */
    if (ntasks_bt1 > 0)
    {
        /* Calcular tamaño de bloque base y cuántos threads tienen un extra */
        int base = ntasks_bt1 / NUM_THREADS;
        int extra = ntasks_bt1 % NUM_THREADS;
        int offset = 0;

        for (int t = 0; t < NUM_THREADS; t++)
        {
            /* Los primeros 'extra' threads reciben base+1 tareas */
            int chunk = base + (t < extra ? 1 : 0);

            args[t].tasks = tasks_bt1;
            args[t].task_start = offset;
            args[t].task_end = offset + chunk;
            offset += chunk;

            pthread_create(&tids[t], NULL, thread_worker, &args[t]);
        }

        /* Esperar a todos los threads y acumular contadores */
        long int local_c8 = 0;
        for (int t = 0; t < NUM_THREADS; t++)
        {
            pthread_join(tids[t], NULL);
            local_c8 += args[t].count8;
        }

        /* Acumular en los contadores globales del proceso MPI */
        GLOBAL_COUNT8 += local_c8; /* se suma antes del MPI_Reduce */
    }

    /* ------------------------------------------------------------------
       PASO 5 — Lanzar threads para el Loop 2 (BT2), igual que el paso 4
       ------------------------------------------------------------------ */
    if (ntasks_bt2 > 0)
    {
        int base = ntasks_bt2 / NUM_THREADS;
        int extra = ntasks_bt2 % NUM_THREADS;
        int offset = 0;

        for (int t = 0; t < NUM_THREADS; t++)
        {
            int chunk = base + (t < extra ? 1 : 0);

            args[t].tasks = tasks_bt2;
            args[t].task_start = offset;
            args[t].task_end = offset + chunk;
            offset += chunk;

            pthread_create(&tids[t], NULL, thread_worker, &args[t]);
        }

        long int local_c8 = 0, local_c4 = 0, local_c2 = 0;
        for (int t = 0; t < NUM_THREADS; t++)
        {
            pthread_join(tids[t], NULL);
            local_c8 += args[t].count8;
            local_c4 += args[t].count4;
            local_c2 += args[t].count2;
        }

        GLOBAL_COUNT8 += local_c8;
        GLOBAL_COUNT4 += local_c4;
        GLOBAL_COUNT2 += local_c2;
    }

    /* ------------------------------------------------------------------
       PASO 6 — MPI_Reduce: cada proceso envía su parcial, rank 0 recibe total
       ------------------------------------------------------------------ */
    long int final_c8, final_c4, final_c2;
    MPI_Reduce(&GLOBAL_COUNT8, &final_c8, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&GLOBAL_COUNT4, &final_c4, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&GLOBAL_COUNT2, &final_c2, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    if (mpi_rank == 0)
    {
        UNIQUE = final_c8 + final_c4 + final_c2;
        TOTAL = final_c8 * 8 + final_c4 * 4 + final_c2 * 2;
    }
}

void liberarEstructuras()
{
    free(tasks_bt1);
    free(tasks_bt2);
    free(args);
    free(tids);
}

/**********************************************/
/* MAIN                                       */
/**********************************************/
int main(int argc, char *argv[])
{
    double tIni, tFin;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    if (argc < 3)
    {
        if (mpi_rank == 0)
            fprintf(stderr, "Uso: mpirun -np <P> %s <N> <num_threads>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    G_SIZE = atoi(argv[1]);
    NUM_THREADS = atoi(argv[2]);

    inicializacionEstructuras();
    tIni = dwalltime();
    NQueens();
    tFin = dwalltime();

    if (mpi_rank == 0)
        printf("Soluciones totales: %ld  —  Tiempo: %.4f s\n", TOTAL, tFin - tIni);

    liberarEstructuras();
    MPI_Finalize();
    return 0;
}

double dwalltime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}
