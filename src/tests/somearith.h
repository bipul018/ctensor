#include <stdio.h>
#include "tensor.h"

// A function to use in the following test
static float subtract_two_floats(float a, float b){
  return a - b;
}

int arith_run(int argc, const char* argv[]){
  (void)argc, (void)argv;
  const Alloc_Interface allocr = gen_std_allocator();

#define DIM 3,4
#define BOOLSTR(boolean) ((boolean)? "Yes" : "No")

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

  // now sliceup and do arithmetic on them
  Tensor t5_s1 = tensor_slice(allocr, t5, (0,1), (2, 3));
  Tensor t5_s2 = tensor_slice(allocr, t5, (1,2), (3, 4));

  printf("\nFirst slice (owner=%s) (Backed by 't5'=%s): \n", BOOLSTR(t5_s1.owner),
	 BOOLSTR(t5_s1.storage.data == t5.storage.data));
  tensor_print(allocr, t5_s1);
  printf("\nSecond slice (owner=%s) (Backed by 't5'=%s): \n", BOOLSTR(t5_s2.owner),
	 BOOLSTR(t5_s2.storage.data == t5.storage.data));
  tensor_print(allocr, t5_s2);

  Tensor t5_ss = tensor_add(allocr, t5_s1, t5_s2);

  printf("\nSum (owner=%s) (Backed by 't5'=%s): \n", BOOLSTR(t5_ss.owner),
	 BOOLSTR(t5_ss.storage.data == t5.storage.data));
  tensor_print(allocr, t5_ss);

  // Now do permutation and arithmetic
  // do (A + A') and (A - A') to make symmetric and anti-symmetric matrices
  Tensor tn_sq = tensor_random(allocr, -2.f, 2.f, 3,3);
  Tensor tn_tr = tensor_permute(allocr, tn_sq, 0, 1);
  Tensor tn_sm = tensor_add(allocr, tn_sq, tn_tr);
  Tensor tn_am = tensor_bin_op(allocr, tn_sq, subtract_two_floats, tn_tr);

  printf("\nThe square matrix A: (owner=%s): \n", BOOLSTR(tn_sq.owner));
  tensor_print(allocr, tn_sq);

  printf("\nTransposition matrix A': (owner=%s) (Backed by 'tn_sq'=%s): \n", BOOLSTR(tn_tr.owner), BOOLSTR(tn_tr.storage.data == tn_sq.storage.data));
  tensor_print(allocr, tn_tr);

  printf("\nSymmetric matrix (A+A'): (owner=%s) (Backed by 'tn_sq'=%s): \n", BOOLSTR(tn_sm.owner), BOOLSTR(tn_sm.storage.data == tn_sq.storage.data));
  tensor_print(allocr, tn_sm);

  printf("\nAnti Symmetric matrix (A-A'): (owner=%s) (Backed by 'tn_sq'=%s): \n", BOOLSTR(tn_am.owner), BOOLSTR(tn_am.storage.data == tn_sq.storage.data));
  tensor_print(allocr, tn_am);

  tensor_free(allocr, &tn_am);
  tensor_free(allocr, &tn_sm);
  tensor_free(allocr, &tn_tr);
  tensor_free(allocr, &tn_sq);
  tensor_free(allocr, &t5_ss);
  tensor_free(allocr, &t5_s2);
  tensor_free(allocr, &t5_s1);
  tensor_free(allocr, &t5);
  tensor_free(allocr, &t4);
  tensor_free(allocr, &t3);
  tensor_free(allocr, &t2);
  tensor_free(allocr, &t1);
#undef DIM
#undef BOOLSTR
  return 0;
}
