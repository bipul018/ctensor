#include <stdio.h>
#include "tensor.h"

static int arith2_run(int argc, const char* argv[]){
  (void)argc, (void)argv;
  const Alloc_Interface allocr = gen_std_allocator();

  // Here, some more arithmetics involving reduction, vectorization are tried out
#define BOOLSTR(boolean) ((boolean)? "Yes" : "No")

  Tensor t1 = tensor_random(allocr, 0.f, 10.f, 2,4,5);
  printf("A random tensor: \n");
  tensor_print(allocr, t1);

  Tensor t2 = tensor_vadd(allocr, 3.f, t1);
  printf("\nVectorized add :\n");
  tensor_print(allocr, t2);

  Tensor t3 = tensor_slice(allocr, t2, (0,0,0), (1,4,5));
  Tensor t4 = tensor_slice(allocr, t2, (1,0,0), (2,4,5));

  Tensor_Iter t3_iter = tensor_iter_init(allocr, t3);
  (void)tensor_vadd(&t3_iter, -3.4f, t4);

  printf("\nAfter slicing and inplace vectorized addition:\n");
  tensor_print(allocr, t2);

  // Now slice more and do binop on the slices
  Tensor t2_s1 = tensor_slice(allocr, t2, (0,0,0), (2, 4, 2));
  Tensor t2_s2 = tensor_slice(allocr, t2, (0,0,2), (2, 4, 4));
  Tensor t2_s3 = tensor_slice(allocr, t2, (0,0,3), (2, 4, 5));

  Tensor_Iter t2s1_iter = tensor_iter_init(allocr, t2_s1);
  (void)tensor_max(&t2s1_iter, t2_s2, t2_s3);

  printf("\nOperand slice 1:\n");
  tensor_print(allocr, t2_s2);
  printf("\nOperand slice 2:\n");
  tensor_print(allocr, t2_s3);

  printf("\nAfter doing max binop within the tensor using slices: \n");
  tensor_print(allocr, t2);
  
  

  tensor_iter_deinit(allocr, &t2s1_iter);
  tensor_free(allocr, &t2_s3);
  tensor_free(allocr, &t2_s1);
  tensor_iter_deinit(allocr, &t3_iter);
  tensor_free(allocr, &t4);
  tensor_free(allocr, &t3);
  tensor_free(allocr, &t2);
  tensor_free(allocr, &t1);
  

#undef BOOLSTR
  return 0;
}
