#include "stdio.h"
#include "omp.h"

void main() {
  #pragma omp parallel
  {
    int id = omp_get_thread_num();
    printf("Helo(%d) world(%d)\n", id, id);
  }
}