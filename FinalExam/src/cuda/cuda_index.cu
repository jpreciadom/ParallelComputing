#include "stdlib.h"
#include "stdio.h"
#include "iostream"
#include "chrono"
#include "time.h"

#include "../utils/matrix/matrix.h"

using namespace std;
using namespace std::chrono;

// Number of threads to execute the Matrix Multiplication
int num_of_blocks;
int threads_per_block;

// Total execution time expressed in milliseconds
double total_execution_time_ms = 0.0;

__global__ void mul_matrixes(long *a, long *b, long *result, int *dimensions)
{
  for (int y = blockIdx.x; y < *dimensions; y+=gridDim.x)
  {
    for (int x = threadIdx.x; x < *dimensions; x+=blockDim.x)
    {
      long current = 0;
      for (int i = 0; i < *dimensions; i++)
      {
        int a_index = (y * (*dimensions)) + i;
        int b_index = (i * (*dimensions)) + x;

        current += ((*(a + a_index)) * (*(b + b_index)));
      }
      int r_index = (y * (*dimensions)) + x;
      *(result + r_index) = current;
    }
  }
}

void setup_mul_matrixes(struct Matrix *a, struct Matrix *b, struct Matrix *result)
{
  cudaError_t cuda_err = cudaSuccess;
  int memory_to_allocate = sizeof(long) * result->area;

  // --------------------------------------------------------------------- //
  // Data setup in device memory
  long *device_a, *device_b, *device_result;
  int *dimensions;

  // Allocate memory in device
  cuda_err = cudaMalloc((void**)&device_a, memory_to_allocate);
  cuda_err = cudaMalloc((void**)&device_b, memory_to_allocate);
  cuda_err = cudaMalloc((void**)&device_result, memory_to_allocate);
  cuda_err = cudaMalloc((void**)&dimensions, sizeof(int));

  // Copy the data to the device memory
  cuda_err = cudaMemcpy(device_a, a->pointer, memory_to_allocate, cudaMemcpyHostToDevice);
  cuda_err = cudaMemcpy(device_b, b->pointer, memory_to_allocate, cudaMemcpyHostToDevice);
  cuda_err = cudaMemcpy(dimensions, &(b->dimensions), sizeof(int), cudaMemcpyHostToDevice);

  // --------------------------------------------------------------------- //
  // Lauch the kernel
  mul_matrixes<<<num_of_blocks, threads_per_block>>>(device_a, device_b, device_result, dimensions);

  // Verify the execution
  cuda_err = cudaGetLastError();
  if (cuda_err)
  {
    perror("Failed to launch distort face kernel");
    exit(EXIT_FAILURE);
  }
  cudaDeviceSynchronize();

  // --------------------------------------------------------------------- //
  // Copy the results into the main memory
  cuda_err = cudaMemcpy(result->pointer, device_result, memory_to_allocate, cudaMemcpyDeviceToHost);

  // Free the device memory
  cudaFree(device_a);
  cudaFree(device_b);
  cudaFree(device_result);
  cudaFree(dimensions);
}

int main(int argc, const char **argv)
{
  // Take the start time
  auto start_time = high_resolution_clock::now();

  // Check the amoung of arguments received
  if (argc != 4)
  {
    cout << "Invalid number of arguments" << endl;
    exit(-1);
  }

  // Fill the variables with the values received via arguments
  num_of_blocks = atoi(*(argv + 1));
  threads_per_block = atoi(*(argv + 2));
  int matrix_size = atoi(*(argv + 3));

  // Create the matrixes
  {
    time_t nTime;
    int seed = (int) time(&nTime);
    setup_seed(seed);
  }
  struct Matrix *matrix_a = generate_matrix(matrix_size, true);
  struct Matrix *matrix_b = generate_matrix(matrix_size, true);
  struct Matrix *matrix_result = generate_matrix(matrix_size, false);

  setup_mul_matrixes(matrix_a, matrix_b, matrix_result);

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
