#include "stdlib.h"
#include "stdio.h"
#include "iostream"
#include "chrono"
#include "time.h"
#include "omp.h"
#include "mpi.h"

#include "../utils/matrix/matrix.h"

using namespace std;
using namespace std::chrono;

// Nodes info
int node_id;
int num_of_nodes;

// Total execution time expressed in milliseconds
double total_execution_time_ms = 0.0;

void mul_matrixes(struct Matrix *a, struct Matrix *b, struct Matrix *result)
{
  // Get the thread id
  int thread_id = node_id;
  int num_of_threads = num_of_nodes;

  // Iterate over the columns, each thread calculate the result of the columns
  // such that column_id % num_of_threads = thread_id and store the results on
  // result matrix.
  for (int ry = thread_id; ry < result->dimensions; ry += num_of_threads)
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

  if (thread_id == 0)
  {
    // Receive from the other nodes their results
    MPI_Status recv_status;
    for (int i = 0; i < result->dimensions; i++)
      if (i % num_of_nodes != 0)
        MPI_Recv(
            result->pointer + (i * result->dimensions),
            result->dimensions,
            MPI_LONG,
            i % num_of_nodes,
            0,
            MPI_COMM_WORLD,
            &recv_status);
  }
  else
    // Send the calculated matrix columns to root node
    for (int y = thread_id; y < result->dimensions; y += num_of_threads)
      MPI_Send(
          result->pointer + (y * result->dimensions),
          result->dimensions,
          MPI_LONG,
          0,
          0,
          MPI_COMM_WORLD);
}

int main(int argc, char **argv)
{
  // Take the start time
  auto start_time = high_resolution_clock::now();

  // Check the amoung of arguments received
  if (argc != 2)
  {
    cout << "Invalid number of arguments" << endl;
    exit(-1);
  }

  // Fill the variables with the values received via arguments
  int matrix_size = atoi(*(argv + 1));

  // Init MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &node_id);
  MPI_Comm_size(MPI_COMM_WORLD, &num_of_nodes);

  // Create the matrixes
  {
    // Generate the seed
    time_t nTime;
    int seed = (int)time(&nTime);
    seed = 0;
    // Give the seed from node 0 to the other nodes
    MPI_Bcast(&seed, 1, MPI_INT, 0, MPI_COMM_WORLD);
    // Set the seed to matrix generator
    setup_seed(0);
  }
  struct Matrix *matrix_a = generate_matrix(matrix_size, true);
  struct Matrix *matrix_b = generate_matrix(matrix_size, true);
  struct Matrix *matrix_result = generate_matrix(matrix_size, false);

  mul_matrixes(matrix_a, matrix_b, matrix_result);

  // Print the matrixes a, b and result
  // if (node_id == 0)
  // {
  //   cout << "Matrix A:" << endl;
  //   print_matrix(matrix_a);
  //   cout << endl;

  //   cout << "Matrix B:" << endl;
  //   print_matrix(matrix_b);
  //   cout << endl;

  //   cout << "Result:" << endl;
  //   print_matrix(matrix_result);
  //   cout << endl;
  // }

  free_matrix(matrix_a);
  free_matrix(matrix_b);
  free_matrix(matrix_result);

  // Take the finish time
  auto finish_time = high_resolution_clock::now();
  // Calculate the algorithm duration time
  auto duration = duration_cast<microseconds>(finish_time - start_time);
  total_execution_time_ms = duration.count();

  // Print the time result
  if (node_id == 0)
  {
    cout
        << "The matrixes were multiplied in "
        << total_execution_time_ms / 1e6
        << " seconds"
        << endl;
  }

  MPI_Finalize();

  return 0;
}
