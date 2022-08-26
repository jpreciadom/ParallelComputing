#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

#define ITERATIONS 2e9
#define TOTAL_THREADS 32
#define ITERATIONS_PER_THREAD ITERATIONS/TOTAL_THREADS

struct pi_calculator_struct {
  int thread_id;
  double result;
};

void *calculate_pi(void *pi_calculator_pointer) {
  struct pi_calculator_struct *pi_calculator = pi_calculator_pointer;

  long first_iteration, last_iteration, current_iteration;
  first_iteration = ITERATIONS_PER_THREAD * pi_calculator->thread_id;
  last_iteration = first_iteration + ITERATIONS_PER_THREAD - 1;
  current_iteration = first_iteration;

  do {
    pi_calculator->result = pi_calculator->result + (4.0 / (double)((current_iteration * 2) + 1));
    current_iteration++;
    pi_calculator->result = pi_calculator->result - (4.0 / (double)((current_iteration * 2) + 1));
    current_iteration++;
  } while (current_iteration < last_iteration);
}

int main() {
  double pi = 0.0;

  struct pi_calculator_struct *pi_calculator_structures = (struct pi_calculator_struct *)malloc(sizeof(struct pi_calculator_struct) * TOTAL_THREADS);
  pthread_t *pi_calculators_ids = (pthread_t *)(malloc(sizeof(pthread_t) * TOTAL_THREADS));

  for (int i = 0; i < TOTAL_THREADS; i++) {
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

  for (int i = 0; i < TOTAL_THREADS; i++) {
    pthread_join(*(pi_calculators_ids + i), NULL);
    pi += (pi_calculator_structures + i)->result;
  }

  printf("\nThe value of pi is: %0.20f\n", pi);
  return 0;
}
