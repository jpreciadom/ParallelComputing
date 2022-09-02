#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#define ITERATIONS 2e9
#define GAP 8

long total_threads;
long iterations_per_thread;

double *pi_results;
int *threads_ids;

void *calculate_pi(void *thread_id_pointer) {
  int thread_id = *(int *)(thread_id_pointer);

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
  threads_ids = (int *)malloc(sizeof(int) * total_threads);
  pthread_t *pi_calculators_ids = (pthread_t *)(malloc(sizeof(pthread_t) * total_threads));

  for (int i = 0; i < total_threads; i++) {
    int *thread_id = (threads_ids + i);
    *thread_id = i;

    pthread_t *pi_calculator_id = (pi_calculators_ids + i);

    int thread_creation_result = pthread_create(
      pi_calculator_id,
      NULL,
      calculate_pi,
      thread_id
    );

    if (thread_creation_result != 0) {
      perror("Error creating the thread\n");
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < total_threads; i++) {
    pthread_join(*(pi_calculators_ids + i), NULL);
    pi += *(pi_results + (i * GAP));
  }

  printf("\nThe value of pi is: %0.8f\n", pi);
  return 0;
}
