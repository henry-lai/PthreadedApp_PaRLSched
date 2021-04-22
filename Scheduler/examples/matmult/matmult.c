#include <stdio.h>
#include <stdlib.h>

double **a;
double **b;
double **res;

int main(int argc, char **argv)
{
  if (argc<3) {
    fprintf (stderr, "Usage: matmult <matrix_dim> <nr_threads> <mem_alloc_type>\n");
    fprintf (stderr, "       <mem_alloc_type> is 0 for non-NUMA allocation (all matrices on one memory node)\n");
    fprintf (stderr, "                           1 for interleaved allocation\n");
    fprintf (stderr, "                           2 for replicated allocation (all matrices replicated on all memory nodes)\n");
    exit(1);
  }

  int dim = atoi(argv[1]);
  int nr_threads = atoi(argv[2]);
  int mem_alloc_type = atoi(argv[3]);
  int i,j,k;

  a = (double **) malloc (sizeof(double *) * dim);
  b = (double **) malloc (sizeof(double *) * dim);
  res = (double **) malloc (sizeof(double *) * dim);
  for (i=0; i<dim; i++) {
    a[i] = (double *) malloc (sizeof(double) * dim);
    b[i] = (double *) malloc (sizeof(double) * dim);
    res[i] = (double *) malloc (sizeof(double) * dim);
    for (j=0; j<dim; j++) {
      a[i][j] = 42.0;
      b[i][j] = 42.0;
    }
  }

  for (i=0; i<dim; i++)
    for (j=0; j<dim; j++) {
      double sum = 0;
      for (k=0; k<dim; k++)
        sum += a[i][k] * b[k][j];
      res[i][j] = sum;
    }

  return 0;
}

