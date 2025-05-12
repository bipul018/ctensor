#include "tensor.h"
#include <stdio.h>


Tensor_Inx tensor_shape(Tensor t){ return (Tensor_Inx){.data = t.shape_base, .count = t.ndim}; }
Tensor_Inx tensor_offset(Tensor t){ return (Tensor_Inx){.data = t.offset_base, .count = t.ndim}; }
Tensor_Inx tensor_stride(Tensor t){ return (Tensor_Inx){.data = t.stride_base, .count = t.ndim}; }
// iif refc is NULL or refc = 0
bool tensor_owner(Tensor t){ return (t.refc == nullptr) || (*t.refc == 0); }


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

#define shape(x) tensor_shape(x)
#define stride(x) tensor_stride(x)
#define offset(x) tensor_offset(x)

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
    .shape_base = SLICE_ALLOC(allocr, uptr, shape.count).data,
    .stride_base = SLICE_ALLOC(allocr, uptr, shape.count).data,
    .offset_base = SLICE_ALLOC(allocr, uptr, shape.count).data,
    .ndim = shape.count,
    // TODO:: Either allocate this here, or use some settings
    .refc = nullptr, // 0 or nullptr means single owner
  };
  MEMCHK(t.storage.data);
  // Since if the shape is 0, the alloc function can return null or not
  //  so check following only if the shape is nonzero length
  if(shape.count > 0){
    MEMCHK(t.shape_base);
    MEMCHK(t.stride_base);
    MEMCHK(t.offset_base);
    // Form the shapes and strides
    memcpy(t.shape_base, shape.data, uptr_slice_bytes(shape));
    (void)memset(t.offset_base, 0, uptr_slice_bytes(offset(t)));
    tensor_force_fix_stride(shape(t), stride(t));
  }
  return t;
}

// To be used because of macro issues
Tensor tensor_assume_contiguous_fix_stride(Tensor in){
  tensor_force_fix_stride(shape(in), stride(in));
  return in;
}


Tensor tensor_create_(Alloc_Interface allocr, f32 fill_elem, Tensor_Inx shape){
  Tensor t = tensor_alloc_(allocr, shape);
  // fill the storage
  for_slice(t.storage, i){
    slice_inx(t.storage, i) = fill_elem;
  }
  return t;
}


f32* tensor_get_ptr_(Tensor t, Tensor_Inx inx){
  // offset = sum(inx_i * stride_i), inx_i < shape_i
  uptr offset = 0;
  // TODO:: Dont assert, return nullptr or something
  assert(((void)"Shape of tensors cannot be different", inx.count == t.ndim));
  for_slice(inx, i){
    assert(((void)"Index must be inside size", inx.data[i] < shape(t).data[i]));
    offset += (offset(t).data[i] + inx.data[i]) * stride(t).data[i];
  }
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
  // TODO:: When refc is actually implemented, also do freeing of variables
  // TODO:: Later need to implement atomic here
  if(t->refc != nullptr){
    if(t->refc[0] == 0){
      SLICE_FREE(allocr, t->storage);
    } else {
      t->refc[0]--;
    }
  }
  free_mem(allocr, t->shape_base);
  free_mem(allocr, t->stride_base);
  free_mem(allocr, t->offset_base);
  (*t) = (Tensor){0};
}

Tensor tensor_dupe(Alloc_Interface allocr, Tensor t){
  Tensor out = {
    .storage = make_copy_f32_slice(allocr, t.storage),
    .shape_base = make_copy_uptr_slice(allocr, shape(t)).data,
    .stride_base = make_copy_uptr_slice(allocr, stride(t)).data,
    .offset_base = make_copy_uptr_slice(allocr, offset(t)).data,
    .ndim = t.ndim,
    //TODO:: Allocate and set this variable later
    .refc = nullptr,
  };
  MEMCHK(out.storage.data);
  // Since if the shape is 0, the alloc function can return null or not
  //  so check following only if the shape is nonzero length
  if(t.ndim > 0){
    MEMCHK(shape(out).data);
    MEMCHK(stride(out).data);
    MEMCHK(offset(out).data);
  }
  return out;
}

Tensor tensor_contiguous(Alloc_Interface allocr, Tensor t){
  // Create equivalent sized tensor
  Tensor newt = tensor_alloc_(allocr, shape(t));

  Tensor_Iter iter = tensor_iter_init(allocr, t);
  while(tensor_iter_next(&iter)){
    *tensor_get_ptr_(newt, iter.inx) = *tensor_get_ptr_(t, iter.inx);
  }
  tensor_iter_deinit(allocr, &iter);

  return newt;
}

Tensor tensor_random_(Alloc_Interface allocr, f32 min_val, f32 max_val, Tensor_Inx shape){
  Tensor rant = tensor_alloc_(allocr, shape);

  for_slice(rant.storage, i){
    rant.storage.data[i] = ((rand() * 1.0) / (RAND_MAX-1)) * (max_val - min_val) + min_val;
  }
  return rant;
  
}

Tensor tensor_range_(Alloc_Interface allocr, f32 start_val, f32 step_size, Tensor_Inx shape){
  Tensor rent = tensor_alloc_(allocr, shape);

  for_slice(rent.storage, i){
    rent.storage.data[i] = start_val;
    start_val += step_size;
  }
  return rent;
}

void tensor_print(Alloc_Interface allocr, Tensor t){
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
      if(slice_inx(iter.inx, i) != (slice_inx(shape(t), i)-1)) break;
      printf("]");
      some_overflowed = true;
    }
    if(some_overflowed) printf("\n");    
  }
  tensor_iter_deinit(allocr, &iter);
}

void tensor_permute_in_place(Tensor t, uptr inx1, uptr inx2){
  assert(((void)"Index out of bounds", inx1 < shape(t).count));
  assert(((void)"Index out of bounds", inx2 < shape(t).count));

  if(inx1 != inx2){
    _swap(shape(t).data[inx1], shape(t).data[inx2]);
    _swap(stride(t).data[inx1], stride(t).data[inx2]);
    _swap(offset(t).data[inx1], offset(t).data[inx2]);
  }
}

Tensor tensor_permute(Alloc_Interface allocr, Tensor oldt, uptr inx1, uptr inx2){
  Tensor newt = {
    .storage = oldt.storage, //shares storage
    .shape_base = make_copy_uptr_slice(allocr, shape(oldt)).data,
    .stride_base = make_copy_uptr_slice(allocr, stride(oldt)).data,
    .offset_base = make_copy_uptr_slice(allocr, offset(oldt)).data,
    .ndim = oldt.ndim,
    // TODO:: Either allocate this here, or use some settings
    .refc = nullptr, // 0 or nullptr means single owner
  };
  assert(("Currently, this is invalid, wont cleanup", false));
  // Since if the shape is 0, the alloc function can return null or not
  //  so check following only if the shape is nonzero length
  if(oldt.ndim > 0){
    MEMCHK(oldt.shape_base);
    MEMCHK(oldt.stride_base);
    MEMCHK(oldt.offset_base);
  }

  tensor_permute_in_place(newt, inx1, inx2);

  return newt;
}

Tensor tensor_slice_(Alloc_Interface allocr, Tensor src,
		     Tensor_Inx start, Tensor_Inx end){
  // Assert if indexes are of valid dimension
  assert(((void)"Dimensions of starting tensor index must be same as tensor dimension",
	  start.count == src.ndim));
  assert(((void)"Dimensions of ending tensor index must be same as tensor dimension",
	  end.count == src.ndim));

  // Assert if the indexes are in valid ranges
  for_slice(shape(src), i){
    assert(((void)"Starting index cannot be greater or equal to size of tensor",
	    start.data[i] < shape(src).data[i]));
    assert(((void)"Starting index cannot be greater than size of tensor",
	    end.data[i] <= shape(src).data[i]));
  }

  // Create new non-owning tensor

  Tensor dst = {
    .storage = src.storage, //shares storage
    .shape_base = make_copy_uptr_slice(allocr, shape(src)).data,
    .stride_base = make_copy_uptr_slice(allocr, stride(src)).data,
    .offset_base = make_copy_uptr_slice(allocr, offset(src)).data,
    .ndim = src.ndim,
    // TODO:: Either allocate this here, or use some settings
    .refc = nullptr, // 0 or nullptr means single owner
  };
  assert(("Currently, this is invalid, wont cleanup", false));
  if(src.ndim>0){
    MEMCHK(shape(dst).data);
    MEMCHK(stride(dst).data);
    MEMCHK(offset(dst).data);
  }
  // Slice-em
  // shape = end - start, offset += start
  for_slice(shape(dst), i){
    shape(dst).data[i] = end.data[i] - start.data[i];
    offset(dst).data[i] += start.data[i];
  }

  return dst;
}

Tensor tensor_map_op_inp(Tensor_Iter* out_iter, Tensor_Slice ts, f32_binop* op){
  // Maybe first assert that there are more than 1`tensors
  assert(((void)"There has to be at least 2 tensors for this operation to have meaning",
	  ts.count >= 2));
  // Then assert that each tensor is of same shape
  for_range(size_t, i, 1, ts.count){
    assert(((void)"Differently shaped tensors cannot be used in elementwise operation", equal_tensor_inx(shape(slice_inx(ts, 0)), shape(slice_inx(ts, i)))));
  }

  // Assert that the output tensor is also of required shape
  assert(((void)"The output tensor should also be of the size of input tensors",
	  equal_tensor_inx(shape(slice_inx(ts,0)), shape(out_iter->t))));
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
  Tensor ans = tensor_alloc_(allocr, shape(slice_inx(ts, 0)));
  Tensor_Iter iter = tensor_iter_init(allocr, ans);
  (void)tensor_map_op_inp(&iter, ts, op);
  tensor_iter_deinit(allocr, &iter);
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
  while(tensor_iter_next(out_iter)){
    *tensor_get_ptr_(out_iter->t, out_iter->inx) = 
      op(sv, *tensor_get_ptr_(tv, out_iter->inx));
  }
  return out_iter->t;
}
Tensor tensor_vector_op_new(Alloc_Interface allocr, f32 sv, f32_binop* op, Tensor tv){
  Tensor ans = tensor_alloc_(allocr, shape(tv));
  Tensor_Iter iter = tensor_iter_init(allocr, ans);
  (void)tensor_vector_op(&iter, sv, op, tv);
  tensor_iter_deinit(allocr, &iter);
  return ans;
}

Tensor tensor_reduce_op_inp(Tensor_Iter* out_iter, Tensor tv, uptr dim, f32_binop* op){
  // For reduce operations to work the number of dimensions should be > 0
  assert(((void)"Cannot do reduction on 0 dimensional tensors", tv.ndim > 0));

  // Assert that the dim is in range
  assert(((void)"The dim to work on should exist in input tensor",
	  dim < tv.ndim));

  // Assert that out_iter's tensor's dim is 1 less
  assert(((void)"The output tensor's dimension count should be 1 less than input",
	  out_iter->t.ndim == (tv.ndim -1)));

  // Assert that the input has at least 1 elem in the dim index
  assert(((void)"The input tensor to reduce must have non-zero dim in the chosen index",
	  slice_inx(shape(tv), dim) > 0));

  // Assert that the out_iter's tensor's shape matches after removing the dim
  for_slice(shape(out_iter->t), i){
    if(i < dim){
      assert(((void)"The dimension of output must match input except for the chosen dimension to work on",
	      slice_inx(shape(tv), i) == slice_inx(shape(out_iter->t), i)));
    } else if(i >= dim){
      assert(((void)"The dimension of output must match input except for the chosen dimension to work on",
	      slice_inx(shape(tv), i+1) == slice_inx(shape(out_iter->t), i)));
    }
  }

  while(tensor_iter_next(out_iter)){
    // Extract the single dimension ni 'tv' corresponding to out_iter
    // Do the reduce operation on that
    uptr offset = 0;
    for_slice(out_iter->inx, i){
      if(i < dim) offset += (offset(tv).data[i] + out_iter->inx.data[i]) * stride(tv).data[i];
      else if(i >= dim) offset += (offset(tv).data[i+1] + out_iter->inx.data[i]) * stride(tv).data[i+1];
    }

    Tensor one_dim = {
      .storage = init_f32_slice(tv.storage.data + offset, tv.storage.count-offset),
      .shape_base = MAKE_ARRAY_SLICE(uptr, slice_inx(shape(tv), dim)).data,
      .stride_base = MAKE_ARRAY_SLICE(uptr, slice_inx(stride(tv), dim)).data,
      .offset_base = MAKE_ARRAY_SLICE(uptr, 0).data,
      .ndim = tv.ndim,
      .refc = nullptr,
    };

    *tensor_get_ptr_(out_iter->t, out_iter->inx) = tensor_get(one_dim, 0);
    for_range(uptr, i, 1, slice_inx(shape(tv), dim)){
      *tensor_get_ptr_(out_iter->t, out_iter->inx) =
      op(*tensor_get_ptr_(out_iter->t, out_iter->inx),
	 tensor_get(one_dim, i));
    }
  }
  
  return out_iter->t;
}

Tensor tensor_reduce_op_new(Alloc_Interface allocr, Tensor tv, uptr dim, f32_binop* op){
  assert(((void)"Input tensor must be at least 1 dimensional",
	  tv.ndim > 0));
  Tensor_Inx out_shape = SLICE_ALLOC(allocr, uptr, tv.ndim - 1);
  if(out_shape.count > 0) MEMCHK(out_shape.data);

  for_slice(out_shape, i){
    if(i < dim) slice_inx(out_shape, i) = slice_inx(shape(tv), i);
    else if(i >= dim) slice_inx(out_shape, i) = slice_inx(shape(tv), i+1);
  }
  Tensor ans = tensor_alloc_(allocr, out_shape);
  SLICE_FREE(allocr, out_shape);

  Tensor_Iter iter = tensor_iter_init(allocr, ans);
  (void)tensor_reduce_op(&iter, tv, dim, op);
  tensor_iter_deinit(allocr, &iter);
  return ans;  
}

Tensor_Iter tensor_iter_init(Alloc_Interface allocr, Tensor t){
  // TODO:: Theoritically, this iterator must either own the tensor 't' or increase ref
  // FIX THAT QUITE SOON, OTHERWISE IT WILL CAUSE A BUG DOWN THE LINE
  Tensor_Iter iter = {
    .t = t,
    .inx = SLICE_ALLOC(allocr, uptr, t.ndim),
    .first_time = true,
  };
  if(t.ndim > 0) MEMCHK(iter.inx.data);
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
      if(slice_inx(iter->inx, i) < slice_inx(shape(iter->t), i)) break;
    }
    if((iter->inx.count == 0) || (slice_inx(iter->inx, 0) >= slice_inx(shape(iter->t), 0))) return false;
    // Modulus the tensor size (a hack, TODO:: remove it)
    for_slice(iter->inx, i) slice_inx(iter->inx, i) = slice_inx(iter->inx, i) % slice_inx(shape(iter->t), i);
    return true;
  }
}




