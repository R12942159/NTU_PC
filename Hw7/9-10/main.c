#include <mpi.h>
#include <stdio.h>
#include <math.h>
#include <stdbool.h>


#define NUM_ITERATIONS 100000

bool is_prime(long long num) {
    if (num < 2) return false;
    for (long long i = 2; i <= sqrt(num); ++i) {
        if (num % i == 0) return false;
    }
    return true;
}

int main(int argc, char *argv[]) {
    int rank, size;
    double elapsed_time;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    elapsed_time = - MPI_Wtime();

    for (int l = 0; l < NUM_ITERATIONS; l++) {
        for (int n = rank + 2; n < 32; n += size) {
            long long mersenne = (1LL << n) - 1;
            if (is_prime(mersenne)) {
                long long perfect_number = mersenne * (1LL << (n - 1));
                if (l == NUM_ITERATIONS - 1){
                    printf("Perfect number(%d): %lld\n", n, perfect_number);
                    fflush(stdout);
                }
            }
        }   
    }

    elapsed_time += MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    // if(!rank){
    //     printf("Elapsed time(%d): %10.6f\n", size, elapsed_time);
    //     fflush(stdout);
    // }

    MPI_Finalize();
    return 0;
}