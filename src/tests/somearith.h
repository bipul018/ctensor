#include <stdio.h>
#include "tensor.h"

int arith_run(int argc, const char* argv[]){
  const Alloc_Interface allocr = gen_std_allocator();

#define DIM 3,4

  Tensor t1 = tensor_random(allocr, 0.f, 100.f, DIM);
  Tensor t2 = tensor_random(allocr, -30.f, 30.f, DIM);
  Tensor t3 = tensor_add(allocr, t1, t2);
  Tensor t4 = tensor_random(allocr, -1.f, 1.f, DIM);
  Tensor t5 = tensor_prod(allocr, t3, t4);

  printf("\n");
  tensor_print(allocr, t1);
  printf("\n+\n");
  tensor_print(allocr, t2);
  printf("\n=\n");
  tensor_print(allocr, t3);
  printf("\n*\n");
  tensor_print(allocr, t4);
  printf("\n=\n");
  tensor_print(allocr, t5);

  tensor_free(allocr, &t5);
  tensor_free(allocr, &t4);
  tensor_free(allocr, &t3);
  tensor_free(allocr, &t2);
  tensor_free(allocr, &t1);
#undef DIM
  return 0;
}
