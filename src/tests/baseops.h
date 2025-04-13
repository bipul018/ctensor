#pragma once
#include <stdio.h>
#include "tensor.h"

int base_run(int argc, const char* argv[]){
  (void)argc, (void)argv;
  const Alloc_Interface allocr = gen_std_allocator();

  Tensor t = tensor_create(allocr, 1.f, 2,3,4);

  printf("\nTensor storage shape: %zu\n", t.storage.count);
  printf("\nShape: ");
  print_tensor_inx(t.shape);
  printf("\nStride: ");
  print_tensor_inx(t.stride);
  printf("\nOffset: ");
  print_tensor_inx(t.offset);

  tensor_get(t, 1,1,1) = 34;
  tensor_get(t, 1,1,2) = 35;
  tensor_get(t, 0,0,0) = 12;
  tensor_get(t, 1,0,0) = 13;
  tensor_get(t, 0,0,1) = -3;

  printf("\nTensor: \n");
  tensor_print(allocr, t);

  tensor_permute_in_place(t, 1, 2);

  printf("\nTensor Yet Again: \n");
  printf("\nShape: ");
  print_tensor_inx(t.shape);
  printf("\nStride: ");
  print_tensor_inx(t.stride);
  printf("\nOffset: ");
  print_tensor_inx(t.offset);
  printf("\n");
  tensor_print(allocr, t);

  printf("\nTensor Yet Again: \n");
  for_range(uptr, i, 0, slice_inx(t.shape, 0)){
    for_range(uptr, j, 0, slice_inx(t.shape, 1)){
      for_range(uptr, k, 0, slice_inx(t.shape, 2)){
	printf("(%zu, %zu, %zu) => %f\t", i, j, k, tensor_get(t, i, j, k));
      }
    }
  }
  printf("\nTensor storage: ");
  for_slice(t.storage, i){
    if(i != 0) printf(", ");
    printf("%f", slice_inx(t.storage, i));
  }

  printf("\n\n");
  Tensor dt = tensor_contiguous(allocr, t);

  printf("\nShape: ");
  print_tensor_inx(dt.shape);
  printf("\nStride: ");
  print_tensor_inx(dt.stride);
  printf("\nOffset: ");
  print_tensor_inx(dt.offset);
  printf("\n");
  tensor_print(allocr, dt);
  printf("\nTensor storage: ");
  for_slice(dt.storage, i){
    if(i != 0) printf(", ");
    printf("%f", slice_inx(dt.storage, i));
  }
  
  
  tensor_free(allocr, &dt);
  tensor_free(allocr, &t);

  return 0;
}
