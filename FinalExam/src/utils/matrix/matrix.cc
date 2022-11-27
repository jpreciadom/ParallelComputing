#include "stdlib.h"
#include "stdio.h"
#include "time.h"
#include "matrix.h"

// Create and fill a matrix of size nxn
struct Matrix * generate_matrix(unsigned int n, bool auto_fill)
{
  // Allocate memory for matrix struct
  struct Matrix *matrix = (struct Matrix *)malloc(sizeof(struct Matrix));
  if (matrix == NULL)
  {
    perror("Error allocating memory for the matrix struct");
    exit(EXIT_FAILURE);
  }

  // Fill the matrix data
  matrix->dimensions = n;
  matrix->area = n*n;
  // Allocate the memory required for the data pointer
  matrix->pointer = (long *)malloc(sizeof(long) * matrix->area);
  if (matrix->pointer == NULL)
  {
    perror("Error allocating memory for the matrix pointer");
    exit(EXIT_FAILURE);
  }

  // Fill the matrix
  if (auto_fill)
  {
    time_t nTime;
    srand((unsigned) time(&nTime));
    for (int i = 0; i < matrix->area; i++)
    {
      *(matrix->pointer + i) = rand() % 100000;
    }
  }

  return matrix;
}

// Print the given matrix
void print_matrix(struct Matrix *matrix)
{
  for (int y = 0; y < matrix->dimensions; y++)
  {
    for (int x = 0; x < matrix->dimensions; x++)
    {
      int index = (y * matrix->dimensions) + x;
      printf("%ld ", *(matrix->pointer + index));
    }
    printf("\n");
  }
}

// Get the value in the matrix at the given position
long get_value(struct Matrix *matrix, int x, int y)
{
  int index = (y * matrix->dimensions) + x;
  return *(matrix->pointer + index);
}

// Set the value in the matrix at the given position
void set_value(struct Matrix * matrix, int x, int y, long value)
{
  int index = (y * matrix->dimensions) + x;
  *(matrix->pointer + index) = value;
}

// Free the memory of the given matrix
void free_matrix(struct Matrix *matrix)
{
  free(matrix->pointer);
  free(matrix);
}
