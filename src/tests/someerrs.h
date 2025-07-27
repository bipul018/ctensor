#pragma once
#include <stdio.h>
#include "tensor.h"

// A dummy allocator allowing a maximum number of active allocations at a time for testing purposes
typedef struct Dummy_Allocator Dummy_Allocator;
struct Dummy_Allocator {
  int max_allocs;
  int curr_allocs;
  Alloc_Interface src;
};

void* dummy_alloc(void* p_data, size_t size, size_t align){
  Dummy_Allocator* alcr = p_data;
  if(alcr->curr_allocs >= alcr->max_allocs) return nullptr;
  void* mem = alloc_mem(alcr->src, size, align);
  if(mem != nullptr) alcr->curr_allocs++;
  return mem;
}
void dummy_free(void* p_data, void* pmem){
  Dummy_Allocator* alcr = p_data;
  if(pmem != nullptr && alcr->curr_allocs > 0) alcr->curr_allocs--;
  free_mem(alcr->src, pmem);
}
Alloc_Interface gen_dummy_allocator(Dummy_Allocator* allocator){
  return (Alloc_Interface){
    .alloc = dummy_alloc,
    .free = dummy_free,
    .p_data = (void*)allocator,
  };
}

int someerrs_run(int argc, const char* argv[]){
  (void)argc, (void)argv;
  const Alloc_Interface gen_allocr = gen_std_allocator();
  const Alloc_Interface dum_allocr = gen_dummy_allocator(&(Dummy_Allocator){
      .max_allocs = 0,
      .curr_allocs = 0,
      .src = gen_allocr,
    });


  init_global_error_chain_pool(gen_allocr, 100);
  // First check for allocation failures when no allocation is allowed
  Tensor t0 = tensor_alloc(dum_allocr, 2,3);
  tensor_print(gen_allocr, t0);

  Tensor t1 = tensor_random(dum_allocr, 1.f, 2.f, 2);
  tensor_print(gen_allocr, t1);

  Tensor t3 = tensor_random(gen_allocr, -1.f, 1.f, 2);
  tensor_print(gen_allocr, t3);

  Tensor t4 = tensor_add(gen_allocr, t3, t1);
  tensor_print(gen_allocr, t4);

  tensor_free(gen_allocr, &t4);
  tensor_free(gen_allocr, &t3);
  tensor_free(dum_allocr, &t1);
  tensor_free(dum_allocr, &t0);

  free_global_error_chain_pool(gen_allocr);
  return 0;
}
