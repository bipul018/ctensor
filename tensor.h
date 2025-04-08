#pragma once
#ifndef TENSOR_H
#define TENSOR_H

#define UTIL_INCLUDE_ALL
#include <util_headers.h>

// Tensor data type -> f32
DEF_SLICE(uptr);
typedef uptr_Slice Tensor_Inx;

void print_tensor_inx(Tensor_Inx);
bool equal_tensor_inx(Tensor_Inx a, Tensor_Inx b);

DEF_SLICE(f32);
typedef struct Tensor Tensor;
struct Tensor {
  f32_Slice storage;
  // TODO:: Here, you have to store 'number of dimensions' twice, fix that
  Tensor_Inx shape;
  Tensor_Inx stride;
  // a flag to denote if this tensor owns the storage too
  // TODO:: Make some external 'manager' later, or make reference counting
  bool owner;
};


void tensor_print(Alloc_Interface allocr, Tensor t);
f32* tensor_inx_ptr_(Tensor t, Tensor_Inx inx);
#define tensor_inx(t, ...)					\
  (*tensor_inx_ptr_((t), MAKE_ARRAY_SLICE(uptr, __VA_ARGS__)))

Tensor tensor_create_(Alloc_Interface allocr, f32 fill_elem, Tensor_Inx shape);
#define tensor_create(allocr, fill_elem, ...)				\
  tensor_create_((allocr), (fill_elem), MAKE_ARRAY_SLICE(uptr, __VA_ARGS__))

// TODO:: Add various types of rngs and dists later
Tensor tensor_random_(Alloc_Interface allocr, f32 min_val, f32 max_val, Tensor_Inx shape);
#define tensor_random(allocr, min_val, max_val, ...)				\
  tensor_random_((allocr), (min_val), (max_val), MAKE_ARRAY_SLICE(uptr, __VA_ARGS__))


void tensor_free(Alloc_Interface allocr, Tensor* t);
uptr tensor_size(Tensor t);

typedef f32 f32_binop(f32 a, f32 b);

Tensor tensor_elemwise_op(Alloc_Interface allocr, Tensor t1, f32_binop* op, Tensor t2);

Tensor tensor_add(Alloc_Interface allocr, Tensor t1, Tensor t2);
Tensor tensor_prod(Alloc_Interface allocr, Tensor t1, Tensor t2);

Tensor tensor_vadd(Alloc_Interface allocr, Tensor t1, f32 f);
Tensor tensor_vprod(Alloc_Interface allocr, Tensor t1, f32 f);
// Modifies dimensions of the original tensor
void tensor_permute(Tensor t, uptr inx1, uptr inx2);

// Creates a new tensor without trying to make it contiguous if original was not
Tensor tensor_dupe(Alloc_Interface allocr, Tensor t);
// Creates a new tensor by always making a new contiguous tensor
Tensor tensor_contiguous(Alloc_Interface allocr, Tensor t);

// An iterator for using tensors
typedef struct Tensor_Iter Tensor_Iter;
struct Tensor_Iter {
  Tensor t;
  Tensor_Inx inx;
  bool first_time;
};

Tensor_Iter tensor_iter_init(Alloc_Interface allocr, Tensor t);
void tensor_iter_deinit(Alloc_Interface allocr, Tensor_Iter* iter);
bool tensor_iter_next(Tensor_Iter* iter);

// Some macros to make life easier
// Only to be used from the macro because standard C cannot return values from scopes
Tensor tensor_assume_contiguous_fix_stride(Tensor in);
#define MEMCHK(...)					\
  assert(("Just Buy More RAM!!!", ( (__VA_ARGS__) != nullptr)))


#define INDEX_ARG_FE(N, X) [X]
#define PROD_FE(N,X)  * (X)
#define JUST_DO_NOTHING(...) __VA_ARGS__

// The 'tensor_elems' is a () enclosed nested {} initialized list of elements
// The varargs is the dimensions of the desired array
#define MAKE_STACK_TENSOR(tensor_elems, ...)				\
  tensor_assume_contiguous_fix_stride					\
  ((Tensor){.storage = {.data = ((f32*)((f32 FOR_EACH_VA(INDEX_ARG_FE, __VA_ARGS__)) JUST_DO_NOTHING tensor_elems)), \
			.count = (1 FOR_EACH_VA(PROD_FE, __VA_ARGS__)),}, \
	    .shape = {.data = ((uptr[]){__VA_ARGS__}),			\
		      .count = VA_NARGS(__VA_ARGS__),},			\
	    .stride = {.data = ((uptr[]){__VA_ARGS__}),			\
		       .count = VA_NARGS(__VA_ARGS__),},		\
  })
   
#endif //TENSOR_H
