#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>


#define NUM_TRIALS 1000
#define MAX_MESSAGE_SIZE (1024 * 1024) // Test 1MB

void ping_pong_test(int num_trials, int rank);

int main(int argc, char *argv[]){
    int p, id;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &p);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);

    if (p != 2 && !id){
        printf("This test requires a process of 2.\n");
        fflush(stdout);
        MPI_Finalize();
        return -1;
    }

    ping_pong_test(NUM_TRIALS, id);

    MPI_Finalize();

    return 0;
}

void ping_pong_test(int num_trials, int id){
    double times[30];
    int message_size[30];
    int size_index = 0;

    for (int size = 1; size <= MAX_MESSAGE_SIZE; size *= 2){
        char *message = (char *) malloc(size * sizeof(char));
        double elapsed_time;
        double total_time = 0.0;

        for (int trial = 0; trial < num_trials; trial++){
            if (!id){
                elapsed_time = - MPI_Wtime();

                MPI_Send(message, size, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
                MPI_Recv(message, size, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                elapsed_time = MPI_Wtime();
                total_time += elapsed_time;
            }else if (id){
                MPI_Recv(message, size, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(message, size, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            }
        }

        if (!id){
            double avg_time = (total_time / num_trials) / 2;
            times[size_index] = avg_time;
            message_size[size_index] = size;
            size_index++;
        }

        free(message);
    }

    if (!id){
        double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_x2 = 0.0;
        int n = size_index;

        for (int i = 0; i < n; i++) {
            double x = message_size[i];
            double y = times[i];
            sum_x += x;
            sum_y += y;
            sum_xy += x * y;
            sum_x2 += x * x;
        }

        double slope = (n * sum_xy - sum_x * sum_y) / (n * sum_x2 - sum_x * sum_x);
        double intercept = (sum_y - slope * sum_x) / n;

        printf("\n估算延遲 (lambda): %.6e 秒\n", intercept);
        fflush(stdout);
        printf("估算頻寬 (B): %.6e bytes/second\n", 1.0 / slope);
        fflush(stdout);
    }
}