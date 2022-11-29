#include "stdlib.h"
#include "stdio.h"
#include "iostream"
#include "chrono"
#include "time.h"
#include "omp.h"

#include "../utils/matrix/matrix.h"

using namespace std;
using namespace std::chrono;

// Number of threads to execute the Matrix Multiplication
int num_of_threads;

// Total execution time expressed in milliseconds
double total_execution_time_ms = 0.0;

void mul_matrixes(struct Matrix *a, struct Matrix *b, struct Matrix *result)
{
  #pragma omp parallel num_threads(num_of_threads)
  {
    // Get the thread id
    int thread_id = omp_get_thread_num();

    // Iterate over the columns, each thread calculate the result of the columns
    // such that column_id % num_of_threads = thread_id and store the results on
    // result matrix.
    for (int ry = thread_id; ry < result->dimensions; ry+=num_of_threads)
    {
      for (int rx = 0; rx < result->dimensions; rx++)
      {
        long current = 0;
        int r_index = (ry * result->dimensions) + rx;
        for (int i = 0; i < result->dimensions; i++)
        {
          int a_index = (ry * result->dimensions) + i;
          int b_index = (i * result->dimensions) + rx;

          current += ((*(a->pointer + a_index)) * (*(b->pointer + b_index)));
        }
        *(result->pointer + r_index) = current;
      }
    }
  }
}

int main(int argc, const char **argv)
{
  // Take the start time
  auto start_time = high_resolution_clock::now();

  // Check the amoung of arguments received
  if (argc != 3)
  {
    cout << "Invalid number of arguments" << endl;
    exit(-1);
  }

  // Fill the variables with the values received via arguments
  num_of_threads = atoi(*(argv + 1));
  int matrix_size = atoi(*(argv + 2));

  // Create the matrixes
  {
    time_t nTime;
    int seed = (int) time(&nTime);
    setup_seed(seed);
  }
  struct Matrix *matrix_a = generate_matrix(matrix_size, true);
  struct Matrix *matrix_b = generate_matrix(matrix_size, true);
  struct Matrix *matrix_result = generate_matrix(matrix_size, false);

  mul_matrixes(matrix_a, matrix_b, matrix_result);

  // cout << "Matrix A:" << endl;
  // print_matrix(matrix_a);
  // cout << endl;

  // cout << "Matrix B:" << endl;
  // print_matrix(matrix_b);
  // cout << endl;

  // cout << "Result:" << endl;
  // print_matrix(matrix_result);
  // cout << endl;

  free_matrix(matrix_a);
  free_matrix(matrix_b);
  free_matrix(matrix_result);

  // Take the finish time
  auto finish_time = high_resolution_clock::now();
  // Calculate the algorithm duration time
  auto duration = duration_cast<microseconds>(finish_time - start_time);
  total_execution_time_ms = duration.count();

  cout
  << "The matrixes were multiplied in "
  << total_execution_time_ms / 1e6
  << " seconds"
  << endl;

  return 0;
}
