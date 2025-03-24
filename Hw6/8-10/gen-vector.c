#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){
    double *a;
    double *ptr;
    FILE *foutptr;

    printf("argv[0] is '%s'\n", argv[0]);
    printf("argv[1] is '%s'\n", argv[1]);
    printf("argv[2] is '%s'\n", argv[2]);

    int n = atoi(argv[1]);

    a = (double *) malloc(n * sizeof(double));
    ptr = a;

    for (int i = 0; i < n; i++){
        *(ptr++) = (double) (i) / (double) (n);
    }

    foutptr = fopen(argv[2], "w");
    fwrite (&n, sizeof(int), 1, foutptr);
    fwrite (a, sizeof(double), n, foutptr);
    fclose (foutptr);
    free(a);


    return 0;
}