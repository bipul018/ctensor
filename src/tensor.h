#pragma once
#ifndef TENSOR_H
#define TENSOR_H

#define UTIL_INCLUDE_ALL
#include <util_headers.h>
#include "errors.h"

// Tensor data type -> f32
DEF_SLICE(uptr);
typedef uptr_Slice Tensor_Inx;

void print_tensor_inx(Tensor_Inx);
bool equal_tensor_inx(Tensor_Inx a, Tensor_Inx b);

DEF_SLICE(f32);

typedef struct Tensor Tensor;
struct Tensor {
  union{
    struct{
      f32_Slice storage;
      // CAREFUL:: The following 3 slices can be null and still be valid (as long as no of dimensions is also 0)
      // TODO:: Here, you have to store 'number of dimensions' thrice, fix that
      Tensor_Inx shape;
      Tensor_Inx stride;
      Tensor_Inx offset;
      // a flag to denote if this tensor owns the storage too
      // TODO:: Make some external 'manager' later, or make reference counting
      bool owner;
    };
    Error_Chain* err;
  };
  // TODO:: Decide if 'default construction = no error' is good or bad long term
  // TODO:: Need to make this struct layout the most efficient storage wise
  bool was_err;
};


// An iterator for using tensors
typedef struct Tensor_Iter Tensor_Iter;
struct Tensor_Iter {
  Tensor t;
  Tensor_Inx inx;
  bool first_time;
};

Tensor_Iter tensor_iter_init(Alloc_Interface allocr, Tensor t);
void tensor_iter_reset(Tensor_Iter* iter);
void tensor_iter_deinit(Alloc_Interface allocr, Tensor_Iter* iter);
bool tensor_iter_next(Tensor_Iter* iter);


void tensor_print(Alloc_Interface allocr, Tensor t);
f32* tensor_get_ptr_(Tensor t, Tensor_Inx inx);
#define tensor_get(t, ...)					\
  (*tensor_get_ptr_((t), MAKE_ARRAY_SLICE(uptr, __VA_ARGS__)))

// Creates a new tensor, storage uninitialized
Tensor tensor_alloc_(Alloc_Interface allocr, Tensor_Inx shape);
#define tensor_alloc(allocr, ...)				\
  tensor_alloc_((allocr), MAKE_ARRAY_SLICE(uptr, __VA_ARGS__))

// Creates a new tensor, storage initialized with given value
Tensor tensor_create_(Alloc_Interface allocr, f32 fill_elem, Tensor_Inx shape);
#define tensor_create(allocr, fill_elem, ...)				\
  tensor_create_((allocr), (fill_elem), MAKE_ARRAY_SLICE(uptr, __VA_ARGS__))

// TODO:: Add various types of rngs and dists later
// Creates a new tensor, storage initialized with uniform random value (using rand())
Tensor tensor_random_(Alloc_Interface allocr, f32 min_val, f32 max_val, Tensor_Inx shape);
#define tensor_random(allocr, min_val, max_val, ...)				\
  tensor_random_((allocr), (min_val), (max_val), MAKE_ARRAY_SLICE(uptr, __VA_ARGS__))

// Creates a new tensor by forming a range
Tensor tensor_range_(Alloc_Interface allocr, f32 start_val, f32 step_size, Tensor_Inx shape);
#define tensor_range(allocr, start_val, step_size, ...)				\
  tensor_range_((allocr), (start_val), (step_size), MAKE_ARRAY_SLICE(uptr, __VA_ARGS__))

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


DEF_SLICE(Tensor);

// Function to wrap an 'Tensor' output using a macro to add additional on-site error reporting
#define tensor_wrap_err(argstr, ...)					\
  tensor_wrap_err_(__FILE__, __func__, __LINE__, argstr, __VA_ARGS__)
Tensor tensor_wrap_err_(const char* file, const char* func, int lineno,
			const char* wrapee_str, Tensor wrappee);

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

#define TENSOR_OP_CHOOSE(name, allocr_or_outiter, ...)		\
  tensor_wrap_err(STRINGIFY(name)"("#__VA_ARGS__")", _Generic((allocr_or_outiter), \
			   Alloc_Interface: CONCAT(name, _new),	\
			   Tensor_Iter*: CONCAT(name, _inp))	\
		  ((allocr_or_outiter), __VA_ARGS__))

typedef f32 f32_binop(f32 a, f32 b);
// Some common fp binop functions to be used as fp operation pointers
// TODO:: Add test cases for all usages of these functions later
f32 f32_add_op(f32 a, f32 b);
f32 f32_prod_op(f32 a, f32 b);
f32 f32_max_op(f32 a, f32 b);
f32 f32_min_op(f32 a, f32 b);


// Need to send in than more one tensors here
//   This is otherwise similar to chaining operations from 'elemwise_op'
TENSOR_OP_DECLFN(tensor_map_op, Tensor_Slice ts, f32_binop* op);
#define tensor_map_op(allocr_or_outiter, in_slice, op_fn)	\
  TENSOR_OP_CHOOSE(tensor_map_op, allocr_or_outiter, in_slice, op_fn)

TENSOR_OP_DECLFN(tensor_bin_op, Tensor t1, f32_binop* op, Tensor t2);
#define tensor_bin_op(allocr_or_outiter, t1, opfn, t2)		\
  TENSOR_OP_CHOOSE(tensor_bin_op, allocr_or_outiter, t1, opfn, t2)

// Following are some convienience functions that just use 'tensor_bin_op' with predefined operation functions
#define tensor_add(allocr_or_outiter, t1, t2) tensor_bin_op(allocr_or_outiter, t1, f32_add_op, t2);
#define tensor_prod(allocr_or_outiter, t1, t2) tensor_bin_op(allocr_or_outiter, t1, f32_prod_op, t2);
#define tensor_max(allocr_or_outiter, t1, t2) tensor_bin_op(allocr_or_outiter, t1, f32_max_op, t2);
#define tensor_min(allocr_or_outiter, t1, t2) tensor_bin_op(allocr_or_outiter, t1, f32_min_op, t2);

// Vectorization like operation, left is scalar, right is elements of the tensor
//Tensor tensor_vector_op(Alloc_Interface allocr, f32 sv, f32_binop* op, Tensor tv);
TENSOR_OP_DECLFN(tensor_vector_op, f32 sv, f32_binop* op, Tensor tv);
#define tensor_vector_op(allocr_or_outiter, scalarv, opfn, tensorv)	\
  TENSOR_OP_CHOOSE(tensor_vector_op, allocr_or_outiter, scalarv, opfn, tensorv)

// Following are some convienience functions that just use 'tensor_vector_op' with predefined operation functions
#define tensor_vadd(allocr_or_outiter, fval, tval) tensor_vector_op(allocr_or_outiter, fval, f32_add_op, tval);
#define tensor_vprod(allocr_or_outiter, fval, tval) tensor_vector_op(allocr_or_outiter, fval, f32_prod_op, tval);
#define tensor_vmax(allocr_or_outiter, fval, tval) tensor_vector_op(allocr_or_outiter, fval, f32_max_op, tval);
#define tensor_vmin(allocr_or_outiter, fval, tval) tensor_vector_op(allocr_or_outiter, fval, f32_min_op, tval);

// Vectorization like operation, but for small tensor and big tensor

// Reduce operation that uses elemwise many op inside
TENSOR_OP_DECLFN(tensor_reduce_op, Tensor tensorv, uptr dim, f32_binop* opfn);
#define tensor_reduce_op(allocr_or_outiter, tensorv, dim, opfn)	\
  TENSOR_OP_CHOOSE(tensor_reduce_op, allocr_or_outiter, tensorv, dim, opfn)

#define tensor_radd(allocr_or_outiter, tval, dim) tensor_reduce_op(allocr_or_outiter, tval, dim, f32_add_op);
#define tensor_rprod(allocr_or_outiter, tval, dim) tensor_reduce_op(allocr_or_outiter, tval, dim, f32_prod_op);
#define tensor_rmax(allocr_or_outiter, tval, dim) tensor_reduce_op(allocr_or_outiter, tval, dim, f32_max_op);
#define tensor_rmin(allocr_or_outiter, tval, dim) tensor_reduce_op(allocr_or_outiter, tval, dim, f32_min_op);

// Creates a new tensor without trying to make it contiguous if original was not
Tensor tensor_dupe(Alloc_Interface allocr, Tensor t);
// Creates a new tensor by always making a new contiguous tensor
Tensor tensor_contiguous(Alloc_Interface allocr, Tensor t);

// Some macros to make life easier
// Only to be used from the macro because standard C cannot return values from scopes
Tensor tensor_assume_contiguous_fix_stride(Tensor in);
#define MEMCHK(...)					\
  assert(((void)"Just Buy More RAM!!!", ( (__VA_ARGS__) != nullptr)))


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
