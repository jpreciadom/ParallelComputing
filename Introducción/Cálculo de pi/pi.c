#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <unistd.h>
#include <errno.h>

struct pi_calculator {
  long first_term;
  long last_term;
  int *threads_finished;
  double *pi;
  sem_t *semaphore;
  pthread_mutex_t *mutex;
};

double calculate_factor(long i) {
  double sign = i % 2 == 0 ? -1.0 : 1.0;
  double factor = 4.0 * sign / (double)((i * 2) - 1);
  return factor;
}

void *calculate_pi(void *pi_calculator_pointer) {
  struct pi_calculator *pi_calculator_structure = pi_calculator_pointer;
  sem_t *semaphore = pi_calculator_structure->semaphore;
  sem_wait(semaphore);

  double pi_reg = 0.0;
  long first_term = pi_calculator_structure->first_term;
  long last_term = pi_calculator_structure->last_term;
  pthread_mutex_t *mutex = pi_calculator_structure->mutex;

  for (long i = first_term; i < last_term; i++) {
    pi_reg += calculate_factor(i);
  }

  pthread_mutex_lock(mutex);
  *(pi_calculator_structure->threads_finished) += 1;
  *(pi_calculator_structure->pi) += pi_reg;
  printf("%d threads have finished\n", *pi_calculator_structure->threads_finished);
  pthread_mutex_unlock(mutex);

  sem_post(semaphore);
}

int main() {
  double pi = 0;
  long terms_to_calculate = 2e9;

  int num_of_threads = 4;
  int max_num_of_threads = 4;
  int threads_finished = 0;
  long terms_per_structure = terms_to_calculate / num_of_threads;
  long current_term = 0;
  struct pi_calculator *pi_calculator_structures = (struct pi_calculator*) malloc(sizeof(struct pi_calculator) * num_of_threads);

  pthread_t *pi_calculators_ids = (pthread_t *)(malloc(sizeof(pthread_t) * num_of_threads));
  sem_t *semaphore = sem_open("PI", O_CREAT, 0700, max_num_of_threads);
  pthread_mutex_t mutex;
  pthread_mutex_init(&mutex, NULL);

  // Uncomment this to delete the semaphore and mutex
  // after interrupt a previous execution

  // sem_close(semaphore);
  // sem_unlink("PI");
  // pthread_mutex_destroy(&mutex);
  // return 0;

  for (int i = 0; i < num_of_threads; i++) {
    struct pi_calculator *pi_calculator_structure = (pi_calculator_structures + i);
    pthread_t *pi_calculator_id = (pi_calculators_ids + i);

    pi_calculator_structure->first_term = current_term + 1;
    current_term += terms_per_structure;
    pi_calculator_structure->last_term = current_term;
    pi_calculator_structure->threads_finished = &threads_finished;
    pi_calculator_structure->pi = &pi;
    pi_calculator_structure->semaphore = semaphore;
    pi_calculator_structure->mutex = &mutex;

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
    printf("%d threads have been launched\n", i + 1);
  }

  printf("\n");

  for (int i = 0; i < num_of_threads; i++) {
    pthread_join(*(pi_calculators_ids + i), NULL);
  }

  sem_close(semaphore);
  sem_unlink("PI");
  pthread_mutex_destroy(&mutex);

  printf("\nThe value of pi is: %0.32f\n", pi);
  return 0;
}
