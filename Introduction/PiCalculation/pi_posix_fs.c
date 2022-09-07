#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#define ITERATIONS 2e9

long total_threads;
long iterations_per_thread;

struct pi_calculator_struct {
  int thread_id;
  double result;
};

void *calculate_pi(void *pi_calculator_pointer) {
  struct pi_calculator_struct *pi_calculator = pi_calculator_pointer;

  long first_iteration, last_iteration, current_iteration;
  first_iteration = iterations_per_thread * pi_calculator->thread_id;
  last_iteration = first_iteration + iterations_per_thread - 1;
  current_iteration = first_iteration;

  do {
    pi_calculator->result = pi_calculator->result + (4.0 / (double)((current_iteration * 2) + 1));
    current_iteration++;
    pi_calculator->result = pi_calculator->result - (4.0 / (double)((current_iteration * 2) + 1));
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

  struct pi_calculator_struct *pi_calculator_structures = (struct pi_calculator_struct *)malloc(sizeof(struct pi_calculator_struct) * total_threads);
  pthread_t *pi_calculators_ids = (pthread_t *)(malloc(sizeof(pthread_t) * total_threads));

  for (int i = 0; i < total_threads; i++) {
    struct pi_calculator_struct *pi_calculator_structure = (pi_calculator_structures + i);
    pi_calculator_structure->thread_id = i;
    pi_calculator_structure->result = 0.0;

    pthread_t *pi_calculator_id = (pi_calculators_ids + i);

    int thread_creation_result = pthread_create(
      pi_calculator_id,
      NULL,
      calculate_pi,
      pi_calculator_structure
    );

    if (thread_creation_result != 0) {
      perror("Error creating the thread\n");
      exit(EXIT_FAILURE);
    }
  }

  for (int i = 0; i < total_threads; i++) {
    pthread_join(*(pi_calculators_ids + i), NULL);
    pi += (pi_calculator_structures + i)->result;
  }

  printf("\nThe value of pi is: %0.8f\n", pi);
  return 0;
}
