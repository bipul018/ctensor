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
  // TODO:: Here, you have to store 'number of dimensions' thrice, fix that
  Tensor_Inx shape;
  Tensor_Inx stride;
  Tensor_Inx offset;
  // a flag to denote if this tensor owns the storage too
  // TODO:: Make some external 'manager' later, or make reference counting
  bool owner;
};


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

void tensor_print(Alloc_Interface allocr, Tensor t);
f32* tensor_get_ptr_(Tensor t, Tensor_Inx inx);
#define tensor_get(t, ...)					\
  (*tensor_get_ptr_((t), MAKE_ARRAY_SLICE(uptr, __VA_ARGS__)))

Tensor tensor_create_(Alloc_Interface allocr, f32 fill_elem, Tensor_Inx shape);
#define tensor_create(allocr, fill_elem, ...)				\
  tensor_create_((allocr), (fill_elem), MAKE_ARRAY_SLICE(uptr, __VA_ARGS__))

// TODO:: Add various types of rngs and dists later
Tensor tensor_random_(Alloc_Interface allocr, f32 min_val, f32 max_val, Tensor_Inx shape);
#define tensor_random(allocr, min_val, max_val, ...)				\
  tensor_random_((allocr), (min_val), (max_val), MAKE_ARRAY_SLICE(uptr, __VA_ARGS__))

// [start, end)
Tensor tensor_slice_(Alloc_Interface allocr, Tensor src, Tensor_Inx start, Tensor_Inx end);
// Send in indexes by wrapping in a bracket 
#define tensor_slice(allocr, tensor, start_inxs, end_inxs)		\
  tensor_slice_((allocr), (tensor),					\
		MAKE_ARRAY_SLICE(uptr, JUST_DO_NOTHING start_inxs),	\
		MAKE_ARRAY_SLICE(uptr, JUST_DO_NOTHING end_inxs))

// Creates a new tensor that shares the storage and has permuted indexes
Tensor tensor_permute(Alloc_Interface allocr, Tensor t, uptr inx1, uptr inx2);

// Seems like this would also be a really important function
void tensor_permute_in_place(Tensor t, uptr inx1, uptr inx2);

void tensor_free(Alloc_Interface allocr, Tensor* t);
uptr tensor_size(Tensor t);

typedef f32 f32_binop(f32 a, f32 b);

DEF_SLICE(Tensor);

// Declares two functions, with a special first argument, and rest arguments
//    according to the passed values in __VA_ARGS__
// First function is suffixed with '_new', and takes in Alloc_Interface as first arg
// Second function is suffixed with '_inp', and takes in Tensor_Iter* as first arg
// First function is supposed to do the operation on a new tensor and return it
// Second function is supposed to do the operation inplace and return it
// This is done because the functions are not supposed to be used directly,
//    but rather through another wrapper macro using _Generic declared using the
//    next macro below
#define TENSOR_OP_DECLFN(name, ...)					\
  Tensor CONCAT(name, _new)(Alloc_Interface allocr, __VA_ARGS__);	\
  Tensor CONCAT(name, _inp)(Tensor_Iter* out_iter, __VA_ARGS__);

// A helper macro to choose operation that both create and do operation in place
#define TENSOR_OP_CHOOSE(name, allocr_or_outiter, ...)	\
  (_Generic((allocr_or_outiter),			\
	    Alloc_Interface: CONCAT(name, _new),	\
	    Tensor_Iter*: CONCAT(name, _inp))		\
   ((allocr_or_outiter), __VA_ARGS__))


Tensor tensor_add(Alloc_Interface allocr, Tensor t1, Tensor t2);
Tensor tensor_prod(Alloc_Interface allocr, Tensor t1, Tensor t2);
Tensor tensor_max(Alloc_Interface allocr, Tensor t1, Tensor t2);
Tensor tensor_min(Alloc_Interface allocr, Tensor t1, Tensor t2);

// Need to send in than more one tensors here
//   This is otherwise similar to chaining operations from 'elemwise_op'
TENSOR_OP_DECLFN(tensor_elemwise_manyop, Tensor_Slice ts, f32_binop* op);
#define tensor_elemwise_manyop(allocr_or_outiter, in_slice, op_fn)	\
  TENSOR_OP_CHOOSE(tensor_elemwise_manyop, allocr_or_outiter, in_slice, op_fn)

TENSOR_OP_DECLFN(tensor_elemwise_op, Tensor t1, f32_binop* op, Tensor t2);
#define tensor_elemwise_op(allocr_or_outiter, t1, opfn, t2)		\
  TENSOR_OP_CHOOSE(tensor_elemwise_op, allocr_or_outiter, t1, opfn, t2)


// Vectorization like operation, left is scalar, right is elements of the tensor
//Tensor tensor_vector_op(Alloc_Interface allocr, f32 sv, f32_binop* op, Tensor tv);
TENSOR_OP_DECLFN(tensor_vector_op, f32 sv, f32_binop* op, Tensor tv);
#define tensor_vector_op(allocr_or_outiter, scalarv, opfn, tensorv)	\
  TENSOR_OP_CHOOSE(tensor_vector_op, allocr_or_outiter, scalarv, opfn, tensorv)

Tensor tensor_vadd(Alloc_Interface allocr, f32 f, Tensor tv);
Tensor tensor_vprod(Alloc_Interface allocr, f32 f, Tensor tv);
Tensor tensor_vmax(Alloc_Interface allocr, f32 f, Tensor tv);
Tensor tensor_vmin(Alloc_Interface allocr, f32 f, Tensor tv);

// Vectorization like operation, but for small tensor and big tensor

// Reduce operation that uses elemwise many op inside
Tensor tensor_reduce_op(Alloc_Interface allocr, Tensor tv, uptr dim, f32_binop* op);

// Creates a new tensor without trying to make it contiguous if original was not
Tensor tensor_dupe(Alloc_Interface allocr, Tensor t);
// Creates a new tensor by always making a new contiguous tensor
Tensor tensor_contiguous(Alloc_Interface allocr, Tensor t);


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
    .shape = {.data = ((uptr[]){__VA_ARGS__}),				\
	      .count = VA_NARGS(__VA_ARGS__),},				\
	      .stride = {.data = ((uptr[]){__VA_ARGS__}),		\
			 .count = VA_NARGS(__VA_ARGS__),},		\
	      .owner = true,						\
	      .offset = {.data = ((uptr[VA_NARGS(__VA_ARGS__)]){0}),	\
			 .count = VA_NARGS(__VA_ARGS__),}		\
	    })
   
#endif //TENSOR_H
