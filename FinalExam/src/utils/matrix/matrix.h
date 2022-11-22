#ifndef MATRIXH
#define MATRIXH

struct Matrix {
  long *pointer;
  unsigned int dimensions;
  unsigned long area;
};

struct Matrix * generate_matrix(unsigned int n);
long get_value(struct Matrix *matrix, int i, int j);
void set_value(struct Matrix * matrix, int i, int j, long value);

#endif
