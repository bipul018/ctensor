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
  printf("\nShape (%zu): ", t0.shape.count);
  print_tensor_inx(t0.shape);
  printf("\nStride (%zu): ", t0.stride.count);
  print_tensor_inx(t0.stride);
  printf("\nOffset (%zu): ", t0.offset.count);
  print_tensor_inx(t0.offset);
  printf("\nTensor: \n");
  tensor_print(allocr, t0);



  tensor_free(allocr, &t0);
  return 0;
}
