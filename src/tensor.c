#define ERRORS_H_IMPL
#include "tensor.h"
#include <stdio.h>

void print_tensor_inx(Tensor_Inx inxs){
  printf("(");
  for_slice(inxs, i){
    if(i != 0) printf(", ");
    printf("%zu", inxs.data[i]);
  }
  printf(")");
}
bool equal_tensor_inx(Tensor_Inx a, Tensor_Inx b){
  if(a.count != b.count) return false;
  for_slice(a, i){
    if(a.data[i] != b.data[i]) return false;
  }
  return true;
}

// Not to be used directly, just a helper fxn
static void tensor_force_fix_stride(Tensor_Inx shape, Tensor_Inx stride){
  for_slice(stride, i_){
    const uptr i = stride.count - i_ - 1;
    if(i_ == 0) {
      stride.data[i] = 1;
      continue;
    }
    stride.data[i] = stride.data[i+1] * shape.data[i+1];
  }
}

// A function to determine if the tensor is valid
//   A valid tensor is the one with no error state, and all the fields allocated appropriately
static bool tensor_valid(Tensor t){
  if(t.was_err) return false;
  if(t.storage.data == nullptr) return false;
  // Since if the shape is 0, the alloc function can return null or not
  //  so check following only if the shape is nonzero length
  if(t.shape.count > 0){
    if(t.shape.data == nullptr) return false;
    if(t.stride.data == nullptr) return false;
    if(t.offset.data == nullptr) return false;
  }
  return true;
}

// Helper macro to use to easily construct the tensor error
#define tensor_err(...)						\
  (Tensor){ .was_err = true, .err = new_error(__VA_ARGS__), }

// To be used in all cases in this library before you use the tensor passed by the user
// TODO:: Make a special compiler macro to ignore all errors 
#define tensor_try(tnsr)					\
  do{								\
    if(!tensor_valid(tnsr)){					\
      return tensor_err(((tnsr).was_err?(tnsr).err:nullptr),	\
			"Tried to use invalid tensor `%s`", #tnsr);	\
    }								\
  }while(0)

// Not to be used directly, just a helper fxn
Tensor tensor_alloc_(Alloc_Interface allocr, Tensor_Inx shape){
  // Find the final size
  uptr size = 1;
  for_slice(shape, s){
    size *= shape.data[s];
  }

  // TODO:: If the shape.count is 0, decide if you want to always make it null explicitly
  // Allocate the tensor resources
  Tensor t = {
    .storage = SLICE_ALLOC(allocr, f32, size),
    .shape = SLICE_ALLOC(allocr, uptr, shape.count),
    .stride = SLICE_ALLOC(allocr, uptr, shape.count),
    .offset = SLICE_ALLOC(allocr, uptr, shape.count),
    .owner = true,
  };
  if(!tensor_valid(t)){
    tensor_free(allocr, &t);
    return tensor_err(nullptr, "Failed to create tensor of dim %zu", shape.count);
  }
  if(shape.count > 0){
    // Form the shapes and strides
    memcpy(t.shape.data, shape.data, uptr_slice_bytes(shape));
    (void)memset(t.offset.data, 0, uptr_slice_bytes(t.offset));
    tensor_force_fix_stride(t.shape, t.stride);
  }
  return t;
}
// To be used because of macro issues
Tensor tensor_assume_contiguous_fix_stride(Tensor in){
  tensor_try(in);
  tensor_force_fix_stride(in.shape, in.stride);
  return in;
}

Tensor tensor_create_(Alloc_Interface allocr, f32 fill_elem, Tensor_Inx shape){
  Tensor t = tensor_alloc_(allocr, shape);
  tensor_try(t);
  // fill the storage
  for_slice(t.storage, i){
    slice_inx(t.storage, i) = fill_elem;
  }
  return t;
}


f32* tensor_get_ptr_(Tensor t, Tensor_Inx inx){
  // TODO:: Find some way of reporting error, or make this private fxn
  if(!tensor_valid(t)) return nullptr;
  // offset = sum(inx_i * stride_i), inx_i < shape_i
  uptr offset = 0;
  // TODO:: Dont assert, find some way to return error chain ?
  assert(((void)"Shape of tensors cannot be different", inx.count == t.shape.count));
  for_slice(inx, i){
    assert(((void)"Index must be inside size", inx.data[i] < t.shape.data[i]));
    offset += (t.offset.data[i] + inx.data[i]) * t.stride.data[i];
  }
  // TODO:: Dont assert, find some way to return error chain ?
  assert(((void)"Should not have happened", offset < t.storage.count));
  return &t.storage.data[offset];
}

bool tensor_inx_in_range(Tensor_Inx inxs, Tensor_Inx shape){
  if(inxs.count != shape.count) return false;
  for_slice(inxs, i){
    if(inxs.data[i] >= shape.data[i]) return false;
  }
  return true;
}

void tensor_free(Alloc_Interface allocr, Tensor* t){
  if(t->was_err){
    free_error(t->err);
  }
  else {
    if(t->owner) SLICE_FREE(allocr, t->storage);
    t->owner = false;
    SLICE_FREE(allocr, t->shape);
    SLICE_FREE(allocr, t->stride);
    SLICE_FREE(allocr, t->offset);
  }
}

Tensor tensor_dupe(Alloc_Interface allocr, Tensor t){
  tensor_try(t);
  Tensor out = {
    .storage = make_copy_f32_slice(allocr, t.storage),
    .shape = make_copy_uptr_slice(allocr, t.shape),
    .stride = make_copy_uptr_slice(allocr, t.stride),
    .offset = make_copy_uptr_slice(allocr, t.offset),
    .owner = true,
  };
  if(!tensor_valid(out)){
    tensor_free(allocr, &out);
    return tensor_err(nullptr, "Failed to create tensor of dim %zu", t.shape.count);
  }
  return out;
}

Tensor tensor_contiguous(Alloc_Interface allocr, Tensor t){
  tensor_try(t);
  // Create equivalent sized tensor
  Tensor newt = tensor_alloc_(allocr, t.shape);
  tensor_try(newt);
  // TODO:: Find a better way to use and manage iterators, maybe a 'pool'???
  Tensor_Iter iter = tensor_iter_init(allocr, t);
  while(tensor_iter_next(&iter)){
    *tensor_get_ptr_(newt, iter.inx) = *tensor_get_ptr_(t, iter.inx);
  }
  tensor_iter_deinit(allocr, &iter);
  return newt;
}

Tensor tensor_random_(Alloc_Interface allocr, f32 min_val, f32 max_val, Tensor_Inx shape){
  Tensor rant = tensor_alloc_(allocr, shape);
  tensor_try(rant);

  for_slice(rant.storage, i){
    rant.storage.data[i] = ((rand() * 1.0) / (RAND_MAX-1)) * (max_val - min_val) + min_val;
  }
  return rant;
  
}

Tensor tensor_range_(Alloc_Interface allocr, f32 start_val, f32 step_size, Tensor_Inx shape){
  Tensor rent = tensor_alloc_(allocr, shape);
  tensor_try(rent);

  for_slice(rent.storage, i){
    rent.storage.data[i] = start_val;
    start_val += step_size;
  }
  return rent;
}

// TODO:: Make it accept a FILE* as parameter
void tensor_print(Alloc_Interface allocr, Tensor t){
  if(!tensor_valid(t)){
    // Print the error chain here
    fprintf(stderr, "Invalid tensor...\n");
    if(t.was_err){
      print_error(stderr, t.err);
    } else {
      fprintf(stderr, "No available error info\n");
    }
    return;
  }
  Tensor_Iter iter = tensor_iter_init(allocr, t);
  while(tensor_iter_next(&iter)){
    size_t zeros = 0;
    for_slice(iter.inx, i_) {
      uptr i = iter.inx.count - i_ - 1;
      if(iter.inx.data[i] == 0) zeros++;
      else break;
    }
    if(zeros > 0){
      for_range(size_t, i, 0, iter.inx.count - zeros) printf(" ");
      for_range(size_t, i, 0, zeros) printf("[");
    }    

    if(iter.inx.count > 0 && zeros == 0) printf(", ");
    printf("%f", *tensor_get_ptr_(t, iter.inx));
    
    bool some_overflowed = false;
    for_slice(iter.inx, i_){
      uptr i = iter.inx.count - i_ - 1;
      if(slice_inx(iter.inx, i) != (slice_inx(t.shape, i)-1)) break;
      printf("]");
      some_overflowed = true;
    }
    if(some_overflowed) printf("\n");    
  }
  tensor_iter_deinit(allocr, &iter);
}

void tensor_permute_in_place(Tensor t, uptr inx1, uptr inx2){
  // TODO:: Find some way of reporting error, or make this private fxn
  if(!tensor_valid(t)) return;
  if(inx1 >= t.shape.count) return;
  if(inx2 >= t.shape.count) return;

  if(inx1 != inx2){
    _swap(t.shape.data[inx1], t.shape.data[inx2]);
    _swap(t.stride.data[inx1], t.stride.data[inx2]);
    _swap(t.offset.data[inx1], t.offset.data[inx2]);
  }
}

Tensor tensor_permute(Alloc_Interface allocr, Tensor oldt, uptr inx1, uptr inx2){
  tensor_try(oldt);
  Tensor newt = {
    .storage = oldt.storage, //shares storage
    .shape = make_copy_uptr_slice(allocr, oldt.shape),
    .stride = make_copy_uptr_slice(allocr, oldt.stride),
    .offset = make_copy_uptr_slice(allocr, oldt.offset),
    .owner = false,
  };
  // TODO:: ensure that all the allocation fxns dont do just 'tensor_try' 
  if(!tensor_valid(newt)){
    tensor_free(allocr, &newt);
    return tensor_err(nullptr, "Failed to create tensor of dim %zu", oldt.shape.count);
  }

  tensor_permute_in_place(newt, inx1, inx2);

  return newt;
}

Tensor tensor_slice_(Alloc_Interface allocr, Tensor src,
		     Tensor_Inx start, Tensor_Inx end){
  tensor_try(src);
  // Assert if indexes are of valid dimension
  if(start.count != src.shape.count){
    return tensor_err(nullptr, "Dimensions of starting tensor index (%zu) != desired tensor dimension (%zu)", start.count, src.shape.count);
  }
  if(end.count != src.shape.count){
    return tensor_err(nullptr, "Dimensions of ending tensor index (%zu) != desired tensor dimension (%zu)", end.count, src.shape.count);
  }

  // Assert if the indexes are in valid ranges
  for_slice(src.shape, i){
    if(start.data[i] >= src.shape.data[i]){
      return tensor_err(nullptr, "Starting index (%zu) >= size of tensor (%zu)",
			start.data[i], src.shape.data[i]);
    }
    if(end.data[i] > src.shape.data[i]){
      return tensor_err(nullptr, "Ending index (%zu) > size of tensor (%zu)",
			end.data[i], src.shape.data[i]);
    }
  }

  // Create new non-owning tensor
  Tensor dst = {
    .storage = src.storage, //shares storage
    .shape = make_copy_uptr_slice(allocr, src.shape),
    .stride = make_copy_uptr_slice(allocr, src.stride),
    .offset = make_copy_uptr_slice(allocr, src.offset),
    .owner = false,
  };

  if(!tensor_valid(dst)){
    tensor_free(allocr, &dst);
    return tensor_err(nullptr, "Failed to create tensor of dim %zu", src.shape.count);
  }

  // Slice-em
  // shape = end - start, offset += start
  for_slice(dst.shape, i){
    dst.shape.data[i] = end.data[i] - start.data[i];
    dst.offset.data[i] += start.data[i];
  }
  return dst;
}

Tensor tensor_map_op_inp(Tensor_Iter* out_iter, Tensor_Slice ts, f32_binop* op){
  if(ts.count < 2)
    return tensor_err(nullptr, "Minimum tensors required for map operation is 2, found %zu", ts.count);
  tensor_try(slice_inx(ts, 0));
  for_range(size_t, i, 1, ts.count){
    // TODO:: Add more info in the error message here
    tensor_try(slice_inx(ts, i));
    if(!equal_tensor_inx(slice_inx(ts, 0).shape, slice_inx(ts, i).shape))
      // Would be cool if you could somehow print the shapes also
      return tensor_err(nullptr, "Tensor at index %zu has different shape from tensor at index %zu for mapping operation", i, 0);
  }
  // TODO:: Decide if you need to also assert if iterator is good to go
  if(!equal_tensor_inx(slice_inx(ts,0).shape, out_iter->t.shape)){
    return tensor_err(nullptr, "Desired outputt tensor has different shape from input tensors");
  }

  while(tensor_iter_next(out_iter)){
    *tensor_get_ptr_(out_iter->t, out_iter->inx) = *tensor_get_ptr_(ts.data[0], out_iter->inx);
    for_range(size_t, i, 1, ts.count){
      *tensor_get_ptr_(out_iter->t, out_iter->inx) =
	op(*tensor_get_ptr_(out_iter->t, out_iter->inx),
	   *tensor_get_ptr_(slice_inx(ts, i), out_iter->inx));
    }
  }
  return out_iter->t;
}

Tensor tensor_map_op_new(Alloc_Interface allocr, Tensor_Slice ts, f32_binop* op){
  if(ts.count == 0) return tensor_err(nullptr, "No tensor supplied as input for map operation");
  Tensor ans = tensor_alloc_(allocr, slice_inx(ts, 0).shape);
  if(!tensor_valid(ans)){
    tensor_free(allocr, &ans);
    return tensor_err(nullptr, "Failed to create tensor of dim %zu", slice_inx(ts, 0).shape.count);
  }
  Tensor_Iter iter = tensor_iter_init(allocr, ans);
  // If the fxn fails, this will be true, as the result tensor is already valid
  Tensor if_err = tensor_map_op_inp(&iter, ts, op);
  tensor_iter_deinit(allocr, &iter);
  // Although the input was valid, the output will be an error, need to not free that 
  if(if_err.was_err){
    tensor_free(allocr, &ans);
    // Need to try on 'if_err' but free 'ans' cuz they are different entities in this branch
    tensor_try(if_err);
  }
  return ans;
}

Tensor tensor_bin_op_new(Alloc_Interface allocr, Tensor t1, f32_binop* op, Tensor t2){
  return tensor_map_op(allocr, MAKE_ARRAY_SLICE(Tensor, t1, t2), op);
}
Tensor tensor_bin_op_inp(Tensor_Iter* out_iter, Tensor t1, f32_binop* op, Tensor t2){
  return tensor_map_op(out_iter, MAKE_ARRAY_SLICE(Tensor, t1, t2), op);
}

f32 f32_add_op(f32 a, f32 b){
  return a + b;
}

f32 f32_prod_op(f32 a, f32 b){
  return a * b;
}

f32 f32_max_op(f32 a, f32 b){
  return ((a>b)?a:b);
};

f32 f32_min_op(f32 a, f32 b){
  return ((a<b)?a:b);
}
Tensor tensor_vector_op_inp(Tensor_Iter* out_iter, f32 sv, f32_binop* op, Tensor tv){
  tensor_try(tv);
  // TODO:: Decide if you need to also assert if iterator is good to go
  while(tensor_iter_next(out_iter)){
    *tensor_get_ptr_(out_iter->t, out_iter->inx) = 
      op(sv, *tensor_get_ptr_(tv, out_iter->inx));
  }
  return out_iter->t;
}
Tensor tensor_vector_op_new(Alloc_Interface allocr, f32 sv, f32_binop* op, Tensor tv){
  tensor_try(tv);
  Tensor ans = tensor_alloc_(allocr, tv.shape);
  tensor_try(ans);
  Tensor_Iter iter = tensor_iter_init(allocr, ans);
  // this operation will succeed as tv is valid, and nothing else is extra factor
  (void)tensor_vector_op(&iter, sv, op, tv);
  tensor_iter_deinit(allocr, &iter);
  return ans;
}

Tensor tensor_reduce_op_inp(Tensor_Iter* out_iter, Tensor tv, uptr dim, f32_binop* op){
  // For reduce operations to work the number of dimensions should be > 0
  if(tv.shape.count == 0){
    return tensor_err(nullptr, "Reduction attepted on a 0 dimensional tensor");
  }
  // Assert that the dim is in range
  if(dim >= tv.shape.count){
    return tensor_err(nullptr, "Invalid choice of dimension (%zu) when tensor is %zu dimensional",
		      dim, tv.shape.count);
  }
  // Assert that out_iter's tensor's dim is 1 less
  if(out_iter->t.shape.count != (tv.shape.count -1)){
    return tensor_err(nullptr, "Expected output tensor of dimension %zu, found %zu",
		      tv.shape.count - 1, out_iter->t.shape.count);
  }
  // Assert that the input has at least 1 elem in the dim index
  if(slice_inx(tv.shape, dim) == 0){
    return tensor_err(nullptr, "Expected at least 1 element in dimension %zu of input tensor, found none",
		      dim);
  }
  // Assert that the out_iter's tensor's shape matches after removing the dim
  for_slice(out_iter->t.shape, i){
    if(i < dim){
      if(slice_inx(tv.shape, i) != slice_inx(out_iter->t.shape, i)){
	return tensor_err(nullptr, "The %zu-th dimension of output mismatches. Expected %zu, got %zu",
			  i, slice_inx(tv.shape,i), slice_inx(out_iter->t.shape, i));
      }
    } else if(i >= dim){
      if(slice_inx(tv.shape, i+1) != slice_inx(out_iter->t.shape, i)){
	return tensor_err(nullptr, "The %zu-th dimension of output mismatches. Expected %zu, got %zu",
			  i, slice_inx(tv.shape,i+1), slice_inx(out_iter->t.shape, i));
      }
    }
  }

  while(tensor_iter_next(out_iter)){
    // Extract the single dimension ni 'tv' corresponding to out_iter
    // Do the reduce operation on that
    uptr offset = 0;
    for_slice(out_iter->inx, i){
      if(i < dim) offset += (tv.offset.data[i] + out_iter->inx.data[i]) * tv.stride.data[i];
      else if(i >= dim) offset += (tv.offset.data[i+1] + out_iter->inx.data[i]) * tv.stride.data[i+1];
    }

    Tensor one_dim = {
      .storage = init_f32_slice(tv.storage.data + offset, tv.storage.count-offset),
      .shape = MAKE_ARRAY_SLICE(uptr, slice_inx(tv.shape, dim)),
      .stride = MAKE_ARRAY_SLICE(uptr, slice_inx(tv.stride, dim)),
      .offset = MAKE_ARRAY_SLICE(uptr, 0),
      .owner = false,
    };

    *tensor_get_ptr_(out_iter->t, out_iter->inx) = tensor_get(one_dim, 0);
    for_range(uptr, i, 1, slice_inx(tv.shape, dim)){
      *tensor_get_ptr_(out_iter->t, out_iter->inx) =
      op(*tensor_get_ptr_(out_iter->t, out_iter->inx),
	 tensor_get(one_dim, i));
    }
  }
  
  return out_iter->t;
}

Tensor tensor_reduce_op_new(Alloc_Interface allocr, Tensor tv, uptr dim, f32_binop* op){
  if(tv.shape.count == 0){
    return tensor_err(nullptr, "Reduction attepted on a 0 dimensional tensor");
  }
  Tensor_Inx out_shape = SLICE_ALLOC(allocr, uptr, tv.shape.count - 1);
  if(out_shape.count > 0 && out_shape.data == nullptr)
    // TODO:: Find out if you need to free out_shape
    // TODO:: Find a way to not have to allocate such temporary shapes all the times
    return tensor_err(nullptr, "Couldnt allocate temporary shape array");
  for_slice(out_shape, i){
    if(i < dim) slice_inx(out_shape, i) = slice_inx(tv.shape, i);
    else if(i >= dim) slice_inx(out_shape, i) = slice_inx(tv.shape, i+1);
  }
  Tensor ans = tensor_alloc_(allocr, out_shape);
  SLICE_FREE(allocr, out_shape);
  if(!tensor_valid(ans)){
    tensor_free(allocr, &ans);
    return tensor_err(nullptr, "Failed to create tensor of dim %zu", tv.shape.count - 1);
  }
  Tensor_Iter iter = tensor_iter_init(allocr, ans);
  // There could be error, if so, a new error tensor is returned
  Tensor if_err = tensor_reduce_op(&iter, tv, dim, op);
  if(if_err.was_err){
    tensor_free(allocr, &ans);
    // Need to try on 'if_err' but free 'ans' cuz they are different entities in this branch
    tensor_try(if_err);
  }
  tensor_iter_deinit(allocr, &iter);
  return ans;  
}

Tensor_Iter tensor_iter_init(Alloc_Interface allocr, Tensor t){
  Tensor_Iter iter = {
    .t = t,
    .inx = SLICE_ALLOC(allocr, uptr, t.shape.count),
    .first_time = true,
  };
  if(t.shape.count > 0) MEMCHK(iter.inx.data);
  return iter;
}

void tensor_iter_deinit(Alloc_Interface allocr, Tensor_Iter* iter){
  SLICE_FREE(allocr, iter->inx);
  *iter = (Tensor_Iter){0};
}
bool tensor_iter_next(Tensor_Iter* iter){
  if(iter->first_time){
    for_slice(iter->inx, i) slice_inx(iter->inx, i) = 0;
    iter->first_time = false;
    // TODO:: Handle the cases where the tensor is of 0 size (not 0 dim)
    // 0 dim tensor should work fine
    return true;
  }
  else{
    for_slice(iter->inx, i_){
      uptr i = iter->inx.count - i_ - 1;
      slice_inx(iter->inx, i) += 1;
      if(slice_inx(iter->inx, i) < slice_inx(iter->t.shape, i)) break;
    }
    if((iter->inx.count == 0) || (slice_inx(iter->inx, 0) >= slice_inx(iter->t.shape, 0))) return false;
    // Modulus the tensor size (a hack, TODO:: remove it)
    for_slice(iter->inx, i) slice_inx(iter->inx, i) = slice_inx(iter->inx, i) % slice_inx(iter->t.shape, i);
    return true;
  }
}




