
# A Tensor Operations Library in C



This library provides an implementation for creating, manipulating, and performing arithmetic operations on tensors in C. This library uses my own 'c utlities library' as a dependency, except for which everything is self-dependent. Below is an overview of the key features and functionality.

Uses 
---

## Features

### 1. **Tensor Creation**
- Create tensors with specified dimensions and initialize them with same value in heap.
- Example:

`Tensor t = tensor_create(allocr, 1.f, 2,3,4);`

- Supports indexing of tensor elements directly via macros as:

```
tensor_inx(t, 1,1,1) = 34;
tensor_inx(t, 0,0,0) = 12;
```

### 2. **Tensor Printing**
- Print tensors in a human-readable format for debugging or visualization.
- Example:

`tensor_print(allocr, t);`

### 3. **Tensor Permutation**
- Change the order of dimensions in a tensor using permutation.
- Example:

`tensor_permute(t, 1, 2);`

### 4. **Arithmetic Operations on Tensors**
Perform element-wise operations on tensors:

- **Addition**: Add two tensors element-wise.

`Tensor t3 = tensor_add(allocr, t1, t2);`

- **Multiplication**: Multiply two tensors element-wise.

`Tensor t5 = tensor_prod(allocr, t3, t4);`

- Generate random tensors within specified ranges:

`Tensor t1 = tensor_random(allocr, 0.f, 100.f, DIM);`

### 5. **Creating Tensors on Stack**
- Create and initialize stack-based tensors with multi-dimensional data.
- Example:

```
Tensor t = MAKE_STACK_TENSOR(({{{1,2}, {2,3}, {4,5}},
{{-3,2}, {-8,7}, {0,-9}},
{{1,8}, {4,-2}, {7,-3}}}),
3,4,2);
```

### 6. **Contiguous Tensor Conversion**
- Convert tensors into contiguous memory layout for efficient storage and access.
- Example:

`Tensor dt = tensor_contiguous(allocr, t);`

### 7. **Tensor Storage Information**
- Retrieve and print storage shape and stride information for tensors.
- Example:

```
printf("\nTensor storage shape: %zu", t.storage.count);
print_tensor_inx(t.shape);
print_tensor_inx(t.stride);
```

---

## Code Demonstrations

### Base Operations (`base_run`)
Demonstrates basic tensor creation and manipulation:
- Create a tensor with dimensions `(2,3,4)`.
- Modify specific elements using indexing.
- Permute dimensions for reshaping.

### Stack Operations (`stack_run`)
Illustrates stack-based tensor creation:
- Create tensors with data directly from multi-dimensional arrays.
- Perform permutation on higher-dimensional tensors.

### Arithmetic Operations (`arith_run`)
Showcases element-wise arithmetic operations:
- Generate random tensors within specified ranges.
- Perform addition and multiplication between two tensors.

---

## Getting Started

To use this library:
1. Ensure the utilities library is downloaded from [[https://github.com/bipul018/c-utils]]

2. Clone this repo or download the files `tensor.h` and `tensor.c`

3. Include the required headers:

`#include "tensor.h"`

4. Use the provided functions to create and manipulate tensors.

5. Compile tensor.c along with your files.

---