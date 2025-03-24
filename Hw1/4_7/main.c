#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_ITERATIONS 10000

int main(int argc, char *argv[]) {
    int rank, size;
    long long local_sum = 0, global_sum;
    double elapsed_time;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    elapsed_time = -MPI_Wtime();

    if (argc != 2) {
        if (!rank) printf("Command line: %s <m>\n", argv[0]);
        MPI_Finalize();
        exit(1);
    }
    int n = atoi(argv[1]);

    for (int l = 0; l < NUM_ITERATIONS; l++) {
        for (long long i = rank + 1; i <= n; i += size) local_sum += i;
        MPI_Reduce(&local_sum, &global_sum, 1, MPI_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    }

    elapsed_time += MPI_Wtime();

    if (!rank) {
        global_sum = global_sum / NUM_ITERATIONS;
        long long formula_sum = (long long)n * (n + 1) / 2;

        printf("Execution time %8.6f\n", elapsed_time);
        fflush(stdout);
        printf("Sum from reduction: %lld\n", global_sum);
        fflush(stdout);
        printf("Formula sum: %lld\n", formula_sum);
        fflush(stdout);
    }

    MPI_Finalize();

    return 0;
}
