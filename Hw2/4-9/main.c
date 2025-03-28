/*
 *   Sieve of Eratosthenes
 *
 *   In this version only odd integers are represented.
 *
 *   Programmed by Michael J. Quinn
 *
 *   Last modification: 4 September 2001
 */

#include "mpi.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define MIN(a,b)  ((a)<(b)?(a):(b))
#define NUM_ITERATIONS 10000

int main (int argc, char *argv[])
{
   double elapsed_time;
   int   els;
   int   global_gap;
   int   high_value;
   int   i, m, count;
   int   id;
   int   index, prime, first, step;
   int   larger_size;
   int   local_count;
   int   low_value;
   char *marked;
   int   num_larger_blocks;
   int   p;
   int   proc0_size;
   int   smaller_size;
   int   size;

   MPI_Init (&argc, &argv);
   MPI_Comm_rank (MPI_COMM_WORLD, &id);
   MPI_Comm_size (MPI_COMM_WORLD, &p);
   elapsed_time = - MPI_Wtime();

   if (argc != 2) {
      if (!id) printf ("Command line: %s <m>\n", argv[0]);
      MPI_Finalize();
      exit (1);
   }

   m = atoi(argv[1]);
   els = (m-1) / 2;       /* Only odd integers will be represented */

   /* Figure out this process's share of the array, as well as the
      integers represented by the first and last array elements */

   smaller_size = els/p;
   larger_size = smaller_size + 1;
   num_larger_blocks = els % p;
   if (id < num_larger_blocks) size = larger_size;
   else size = smaller_size;
   low_value = 2*(id*smaller_size + MIN(id, num_larger_blocks)) + 3;
   high_value = low_value + 2*(size - 1);

   /* Increase overlap beyond processor 0 */
   if (id) {
      low_value -= 200; 
      size += 100;
   }
   /* Increase overlap beyond processor 0 */

   if (num_larger_blocks > 0) proc0_size = larger_size;
   else proc0_size = smaller_size;

   if ((1 + 2*proc0_size) < (int) sqrt((double)m)) {
      if (!id) printf ("Too many processes, given upper bound of sieve\n");
      if (!id) printf ("proc0_size is %d and m is %d\n", proc0_size, m);
      MPI_Finalize();
      exit (1);
   }

   /* Allocate this process's share of the array. */
   for (int l = 0; l < NUM_ITERATIONS; l++) {
      marked = (char *) malloc (size);

      if (marked == NULL) {
         printf ("Cannot allocate enough memory\n");
         MPI_Finalize();
         exit (1);
      }

      for (i = 0; i < size; i++) marked[i] = 0;
      if (!id) index = 0;
      prime = 3;
      do {
         if (prime * prime > low_value) first = (prime * prime - low_value)/2;
         else {
            int r = low_value % prime;
            if (!r) first = 0;
            else if ((prime - r) & 1)
               first = (2*prime - r)/2;
            else first = (prime - r)/2;
         }
         for (i = first; i < size; i += prime) marked[i] = 1;
         if (!id) {
            while (marked[++index]);
            prime = 2*index + 3;
         }
         MPI_Bcast (&prime,  1, MPI_INT, 0, MPI_COMM_WORLD);
      } while (prime * prime <= m);

      count = 0;
      int prv_prime = -1;
      int local_gap = 0;
      int gap;
      for (i = 0; i < size; i++)
         if (prv_prime != -1 && !marked[i]){
            gap = 2 * (count + 1);
            count = 0;
            if (gap > local_gap) local_gap = gap;
         } else if (prv_prime == -1){
            prv_prime = marked[i];
         } else count ++;
      MPI_Reduce (&local_gap, &global_gap, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

      free (marked);
   }

   elapsed_time += MPI_Wtime();
   elapsed_time /= NUM_ITERATIONS;

   if (!id) {
      printf ("The maximum gap between consecutive prime numbers is %d\n",
         global_gap);
      printf ("SIEVE_ODD (%d) %10.6f\n", p, elapsed_time);
   }
   MPI_Finalize ();
   return 0;
}
