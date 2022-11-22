#include "stdlib.h"
#include "stdio.h"
#include "matrix.h"

struct Matrix * generate_matrix(unsigned int n)
{
  struct Matrix *matrix = (struct Matrix *)malloc(sizeof(struct Matrix));
  if (matrix == NULL)
  {
    perror("Error allocating memory for the matrix struct");
    exit(EXIT_FAILURE);
  }

  matrix->dimensions = n;
  matrix->area = n*n;
  matrix->pointer = (long *)malloc(sizeof(long) * matrix->area);
  if (matrix->pointer == NULL)
  {
    perror("Error allocating memory for the matrix pointer");
    exit(EXIT_FAILURE);
  }

  // Fill the matrix

  return matrix;
}
