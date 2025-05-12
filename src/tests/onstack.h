#include <stdio.h>
#include "tensor.h"

int stack_run(int argc, const char* argv[]){
  (void)argc, (void)argv;
  const Alloc_Interface allocr = gen_std_allocator();

  {
    printf("Example with 3D tensor\n");
    Tensor t = MAKE_STACK_TENSOR(({{{1,2}, {2,3}, {3,4}, {4,5}},
				   {{-3,2}, {-8,7}, {-9,8}, {0,-9}},
				   {{1,8}, {4,-2}, {7,-3}, {3,3}}}),
      3,4,2);
    printf("\nTensor storage shape: %zu", t.storage.count);
    printf("\nShape: ");
    print_tensor_inx(tensor_shape(t));
    printf("\nStride: ");
    print_tensor_inx(tensor_stride(t));
    printf("\nOffset: ");
    print_tensor_inx(tensor_offset(t));

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
    tensor_permute_in_place(t, 2,3);
    printf("\nTensor storage shape: %zu", t.storage.count);
    printf("\nShape: ");
    print_tensor_inx(tensor_shape(t));
    printf("\nStride: ");
    print_tensor_inx(tensor_stride(t));
    printf("\nOffset: ");
    print_tensor_inx(tensor_offset(t));

    printf("\nTensor: \n");
    tensor_print(allocr, t);
  }
  
  return 0;

}
