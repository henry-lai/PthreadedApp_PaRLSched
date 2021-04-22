#include <stdio.h>
#include <stdlib.h>

#if defined(USE_TBB)
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
#include <tbb/task_scheduler_init.h>

using namespace tbb;
#endif


double **a;
double **b;
double **res;
int dim;

double multiply_row_by_column (double **mat1, int row, double **mat2, int col)
{
  int k;
  double sum=0;
  for (k=0; k<dim; k++)
    sum += mat1[row][k] * mat2[k][col];
}


void multiply_row_by_matrix (double **mat1, int row, double **mat2, double **res)
{
  int i;
  for (col=0; col<dim; col++) 
    res[row][col] = multiply_row_by_column (mat1, row, mat2, col);
  
}

#if defined(USE_TBB)
class MultiplyRowMatrix {
  double **mat1;
  double **mat2;
  double **res;
public:
  void operator() (const blocked_range<size_t> &r) const {
    for (size_t i=r.begin(); i!=r.end(); i++)
      multiply_row_by_matrix (mat1, i, mat2, res);
  }

  MultiplyRowMatrix(double **mat1, double **mat2, double **res) :
    mat1(mat1), mat2(mat2), res(res)
  {}
  
};
#endif
  
int main(int argc, char **argv)
{
  if (argc<3) {
    fprintf (stderr, "Usage: matmult <matrix_dim> <nr_threads> <mem_alloc_type>\n");
    fprintf (stderr, "       <mem_alloc_type> is 0 for non-NUMA allocation (all matrices on one memory node)\n");
    fprintf (stderr, "                           1 for interleaved allocation\n");
    fprintf (stderr, "                           2 for replicated allocation (all matrices replicated on all memory nodes)\n");
    exit(1);
  }

  dim = atoi(argv[1]);
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

#if defined(SEQ)  
  for (i=0; i<dim; i++)
    multiply_row_by_matrix (a, i, b, res);
#elif defined(USE_TBB)
  parallel_for(block_range<size_t>(0,dim), MulitplyRowMatrix(a,b,res));
#endif
  
  return 0;
}

