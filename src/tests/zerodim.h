#pragma once
#include <stdio.h>
#include "tensor.h"

int zerodim_run(int argc, const char* argv[]){
  (void)argc, (void)argv;
  const Alloc_Interface allocr = gen_std_allocator();
  (void)allocr;

  Tensor t0 = tensor_alloc(allocr, );

  tensor_get(t0) = 761.f;

  printf("\nTensor storage shape: %zu\n", t0.storage.count);
  printf("\nShape (%zu): ", t0.ndim);
  print_tensor_inx(tensor_shape(t0));
  printf("\nStride (%zu): ", t0.ndim);
  print_tensor_inx(tensor_stride(t0));
  printf("\nOffset (%zu): ", t0.ndim);
  print_tensor_inx(tensor_offset(t0));
  printf("\nTensor: \n");
  tensor_print(allocr, t0);



  tensor_free(allocr, &t0);
  return 0;
}
