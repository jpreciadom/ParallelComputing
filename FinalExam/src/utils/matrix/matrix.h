#ifndef MATRIXH
#define MATRIXH

struct Matrix {
  long *pointer;
  unsigned int dimensions;
  unsigned long area;
};

void setup_seed(int seed);
struct Matrix * generate_matrix(unsigned int n, bool auto_fill);
void print_matrix(struct Matrix *matrix);
long get_value(struct Matrix *matrix, int x, int y);
void set_value(struct Matrix * matrix, int x, int y, long value);
void free_matrix(struct Matrix *matrix);

#endif
