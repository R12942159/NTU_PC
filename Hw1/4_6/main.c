#include <mpi.h>
#include <stdio.h>

#define NUM_ITERATIONS 1000


int main(int argc, char *argv[]){
    int id;
    int p;
    double elapsed_time;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    MPI_Comm_size(MPI_COMM_WORLD, &p);

    elapsed_time = - MPI_Wtime();

    for (int i = 0; i < NUM_ITERATIONS; i++){
        printf("hello, world, from process %d \n", id);
        fflush(stdout);    
    }

    elapsed_time += MPI_Wtime();
    elapsed_time /= NUM_ITERATIONS;
    
    if (!id) {
        printf ("Exercise 4-6 executive time for %d process: %10.6f \n", p, elapsed_time);
    }
    
    MPI_Finalize();

    return 0;
}