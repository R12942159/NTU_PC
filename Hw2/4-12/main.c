#include "mpi.h"
#include <math.h>
#include <stdio.h>

#define N 50
#define NUM_ITERATIONS 1000000


double f(int i){
    double x;
    x = (double) i / (double) N;
    return 4.0 / (1.0 + x * x);
}

int main(int argc, char *argv[]) {
    double global_area, local_area;
    int i;
    int rank, size;
    double elapsed_time;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    elapsed_time = - MPI_Wtime();

    for (int l = 0; l < NUM_ITERATIONS; l++) {
        local_area = 0.0;
        global_area = 0.0;
        for (i = rank + 1; i <= N / 2; i += size){
            local_area += 4.0 * f(2 * i - 1) + 2.0 * f(2 * i);
        }

        if (!rank){
            local_area += f(0) - f(N);
        }   

        MPI_Reduce(&local_area, &global_area, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    }

    elapsed_time += MPI_Wtime();

    if(!rank){
        global_area /= (3.0 * N);
        printf("Approximation of pi : %13.11f\n", global_area);
        fflush(stdout);
        printf("Elapsed time(%d): %10.6f\n", size, elapsed_time);
        fflush(stdout);
    }

    MPI_Finalize();
    return 0;
}