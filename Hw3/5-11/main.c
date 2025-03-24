#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_ITERATIONS 10000

int main(int argc, char *argv[]){
    int rank, size;
    double elapsed_time;
    double harmonic_sums;
    double global_sums;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    elapsed_time = - MPI_Wtime();

    if (argc != 2){
        if (!rank) printf("Command line: %s <m>\n", argv[0]);
        MPI_Finalize();
        exit(1);
    }

    int n = atoi(argv[1]);

    harmonic_sums = 0.0;
    for (int l = 0; l < NUM_ITERATIONS; l++){
        for (int i = rank+1; i <= n; i += size)
            harmonic_sums += 1.0 / (double)i;
    }
    harmonic_sums /= NUM_ITERATIONS;

    MPI_Reduce(&harmonic_sums, &global_sums, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    elapsed_time += MPI_Wtime();

    if (!rank){
        printf("Execution time %8.6f\n", elapsed_time);
        fflush(stdout);
    }
    
    MPI_Finalize();

    if (!rank){
        printf ("The solution is %.100f\n", global_sums);
        fflush(stdout);
    }
    
    return 0;
}