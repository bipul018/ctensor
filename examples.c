#include <stdio.h>
#include "tensor.h"

void base_run(void);
void stack_run(void);
void arith_run(void);

int main(void){
  printf("\n#########################################################################\n");
  base_run();
  printf("\n#########################################################################\n");
  stack_run();
  printf("\n#########################################################################\n");
  printf("\n#########################################################################\n");
  arith_run();
  printf("\n#########################################################################\n");
  return 0;
}
 

void arith_run(void){
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





  tensor_free(allocr, &t1);
  tensor_free(allocr, &t2);
  tensor_free(allocr, &t3);
}

void stack_run(void){
  const Alloc_Interface allocr = gen_std_allocator();

  {
    printf("Example with 3D tensor\n");
    Tensor t = MAKE_STACK_TENSOR(({{{1,2}, {2,3}, {DIM}, {4,5}},
				   {{-3,2}, {-8,7}, {-9,8}, {0,-9}},
				   {{1,8}, {4,-2}, {7,-3}, {3,3}}}),
      3,4,2);
    printf("\nTensor storage shape: %zu", t.storage.count);
    printf("\nShape: ");
    print_tensor_inx(t.shape);
    printf("\nStride: ");
    print_tensor_inx(t.stride);

    printf("\nTensor: \n");
    tensor_print(allocr, t);
  }
  {
    printf("Example with 4D tensor with permutation\n");
    Tensor t = MAKE_STACK_TENSOR(({{{{1,2}, {2,3}, {3,4}, {4,5}},
				    {{-3,2}, {-8,7}, {-9,8}, {0,-9}},
				    {{1,8}, {4,-2}, {7,-3}, {3,3}}},
				   {{},
				    {{-34,-35}}}}),
      2,3,4,3);
    tensor_permute(t, 2,3);
    printf("\nTensor storage shape: %zu", t.storage.count);
    printf("\nShape: ");
    print_tensor_inx(t.shape);
    printf("\nStride: ");
    print_tensor_inx(t.stride);

    printf("\nTensor: \n");
    tensor_print(allocr, t);
  }
  
		

}


void base_run(void){

  const Alloc_Interface allocr = gen_std_allocator();

  Tensor t = tensor_create(allocr, 1.f, 2,3,4);

  printf("\nTensor storage shape: %zu\n", t.storage.count);
  printf("\nShape: ");
  print_tensor_inx(t.shape);
  printf("\nStride: ");
  print_tensor_inx(t.stride);


  tensor_inx(t, 1,1,1) = 34;
  tensor_inx(t, 1,1,2) = 35;
  tensor_inx(t, 0,0,0) = 12;
  tensor_inx(t, 1,0,0) = 13;
  tensor_inx(t, 0,0,1) = -3;

  printf("\nTensor: \n");
  tensor_print(allocr, t);
  tensor_permute(t, 1, 2);

  printf("\nTensor Yet Again: \n");
  printf("\nShape: ");
  print_tensor_inx(t.shape);
  printf("\nStride: ");
  print_tensor_inx(t.stride);
  printf("\n");
  tensor_print(allocr, t);

  printf("\nTensor Yet Again: \n");
  for_range(uptr, i, 0, slice_inx(t.shape, 0)){
    for_range(uptr, j, 0, slice_inx(t.shape, 1)){
      for_range(uptr, k, 0, slice_inx(t.shape, 2)){
	printf("(%zu, %zu, %zu) => %f\t", i, j, k, tensor_inx(t, i, j, k));
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
  printf("\n");
  tensor_print(allocr, dt);
  printf("\nTensor storage: ");
  for_slice(dt.storage, i){
    if(i != 0) printf(", ");
    printf("%f", slice_inx(dt.storage, i));
  }
  
  
  tensor_free(allocr, &dt);
  tensor_free(allocr, &t);
}
