#pragma once
#include <stdio.h>
#include "tensor.h"

#define BOOLSTR(boolean) ((boolean)? "Yes" : "No")

int viewops_run(int argc, const char* argv[]){
  const Alloc_Interface allocr = gen_std_allocator();

  // Create a single tensor with original storage

  // Create various views and print them
  Tensor t_og = tensor_random(allocr, 0.f, 1.f, 2,5,4);

  printf("\nTensor storage shape: %zu\n", t_og.storage.count);
  printf("\nShape: ");
  print_tensor_inx(t_og.shape);
  printf("\nStride: ");
  print_tensor_inx(t_og.stride);
  printf("\nOffset: ");
  print_tensor_inx(t_og.offset);
  printf("\nOwns Storage: %s", BOOLSTR(t_og.owner));
  printf("\nTensor: \n");
  tensor_print(allocr, t_og);

  Tensor tp1 = tensor_permute(allocr, t_og, 1, 2);
  printf("\nPermuted Tensor: \n");
  printf("\nShape: ");
  print_tensor_inx(tp1.shape);
  printf("\nStride: ");
  print_tensor_inx(tp1.stride);
  printf("\nOffset: ");
  print_tensor_inx(tp1.offset);
  printf("\nOwns Storage: %s", BOOLSTR(tp1.owner));
  printf("\n");
  tensor_print(allocr, tp1);

  printf("\nSliced Tensor: \n");
  Tensor ts1 = tensor_slice(allocr, t_og, (0,1,1), (2, 2, 3));
  printf("\nShape: ");
  print_tensor_inx(ts1.shape);
  printf("\nStride: ");
  print_tensor_inx(ts1.stride);
  printf("\nOffset: ");
  print_tensor_inx(ts1.offset);
  printf("\nOwns Storage: %s", BOOLSTR(ts1.owner));
  printf("\n");
  tensor_print(allocr, ts1);

  // Modifying slice
  tensor_get(ts1, 1,0,1) = 969.f;

  printf("\nSlice after modification: \n");
  tensor_print(allocr, ts1);
  printf("\nOriginal tensor after modification: \n");
  tensor_print(allocr, t_og);

  // Permutation of slice 
  printf("\nPermutation of the slice: \n");
  Tensor tp2 = tensor_permute(allocr, ts1, 0,1);
  printf("\nShape: ");
  print_tensor_inx(tp2.shape);
  printf("\nStride: ");
  print_tensor_inx(tp2.stride);
  printf("\nOffset: ");
  print_tensor_inx(tp2.offset);
  printf("\nOwns Storage: %s", BOOLSTR(tp2.owner));
  printf("\n");
  tensor_print(allocr, tp2);
  
  // Taking slice again
  printf("\nReslicing of the slice: \n");
  Tensor ts2 = tensor_slice(allocr, tp2, (0,0,1,), (1,2,2));
  printf("\nShape: ");
  print_tensor_inx(ts2.shape);
  printf("\nStride: ");
  print_tensor_inx(ts2.stride);
  printf("\nOffset: ");
  print_tensor_inx(ts2.offset);
  printf("\nOwns Storage: %s", BOOLSTR(ts2.owner));
  printf("\n");
  tensor_print(allocr, ts2);
  
  tensor_free(allocr, &ts2);
  tensor_free(allocr, &tp2);
  tensor_free(allocr, &ts1);
  tensor_free(allocr, &tp1);
  tensor_free(allocr, &t_og);

  return 0;
}
