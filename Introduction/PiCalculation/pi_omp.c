#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <omp.h>

#define ITERATIONS 2e9
#define GAP 8

long total_threads;
long iterations_per_thread;

double *pi_results;

void calculate_pi(int thread_id) {
  long first_iteration, last_iteration, current_iteration;
  first_iteration = iterations_per_thread * thread_id;
  last_iteration = first_iteration + iterations_per_thread - 1;
  current_iteration = first_iteration;

  double *pi_result = pi_results + (thread_id * GAP);
  *pi_result = 0.0;
  do {
    *pi_result = *pi_result + (4.0 / (double)((current_iteration * 2) + 1));
    current_iteration++;
    *pi_result = *pi_result - (4.0 / (double)((current_iteration * 2) + 1));
    current_iteration++;
  } while (current_iteration < last_iteration);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    printf("One argument required, num of threads\n");
    exit(-1);
  }
  total_threads = atol(argv[1]);
  iterations_per_thread = ITERATIONS / total_threads;

  double pi = 0.0;
  pi_results = (double *)malloc(sizeof(double) * total_threads * GAP);

  #pragma omp parallel num_threads(total_threads)
  {
    int thread_id = omp_get_thread_num();
    calculate_pi(thread_id);
  }

  for (int i = 0; i < total_threads; i++) {
    pi += *(pi_results + (i * GAP));
  }

  printf("\nThe value of pi is: %0.8f\n", pi);
  return 0;
}
