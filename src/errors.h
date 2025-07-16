#ifndef ERRORS_H
#define ERRORS_H


#define UTIL_INCLUDE_ALL
#include <util_headers.h>

typedef struct Error_Chain Error_Chain;
struct Error_Chain{
  // A null terminated error message allocated from a global pool somewhere (preferably)
  const char* err_msg;
  // nullptr => end of the error chain,
  // -1 => a free error node (used for creating error pool)
  Error_Chain* prev;
};
DEF_SLICE(Error_Chain);
struct Global_Error_Chain_Pool;
extern const struct Global_Error_Chain_Pool* global_error_pool;


// When making a new error chain node, prefer to rather erase the head and use it as tail
//   rather than returning error (It probably is better to report the latest error in the chain)
// Only if this is a fresh error, then you can return nullptr (or you probably can also assert that part)
void init_global_error_chain_pool(Alloc_Interface allocr, size_t count);
void free_global_error_chain_pool(Alloc_Interface allocr);
Error_Chain* new_error(Error_Chain* prev_err, const char* file_func, int lineno, const char* fmt, ...);
// TODO:: Find out if this is even worth doing at all
void free_error_chain(Error_Chain* err_chain);

#ifdef ERRORS_H_IMPL

DEF_SLICE(size_t);
typedef struct Error_Buffer_Unit Error_Buffer_Unit;
struct Error_Buffer_Unit{
  Error_Buffer_Unit* next;
  size_t size;
};
DEF_SLICE(Error_Buffer_Unit);
static struct {
  Error_Chain_Slice pool;
  size_t curr;
  Error_Buffer_Unit_Slice msg_pool;
  size_t msg_off;
} global_error_pool_impl = {0};
const struct Global_Error_Chain_Pool* global_error_pool = &global_error_pool_impl;

Error_Chain* new_error(Error_Chain* prev_err, const char* file_func, int lineno, const char* fmt, ...){
  // First find out if any error is free
  
  Error_Chain* perr = nullptr;
  for_slice(global_error_pool_impl.pool, i){
    Error_Chain* maybe = &slice_inx(global_error_pool_impl.pool,
				    (i+global_error_pool_impl.curr)%
				    global_error_pool_impl.pool.count);
    if(maybe->prev == (Error_Chain*)(-1)){
      perr = maybe;
      global_error_pool_impl.curr+=i;
      global_error_pool_impl.curr%=global_error_pool_impl.pool.count;
      break;
    }
  }
  // If it is not so, see if prev_err is not null
  if(perr == nullptr){
    //    If the prev_err contains some in the chain, then pick out the topmost error and use it to build the current error
    // Else if prev_err is indeed null, then maybe assert, or return null
  
    // TODO:: Later properly decide about this after a long thought 
    assert(("Oh, no!!! Couldn't even return error", prev_err != nullptr));
    Error_Chain** pperr = &prev_err;
    while((*pperr)->prev != nullptr){
      pperr = &(*pperr)->prev;
    }
    perr = *pperr;
    *pperr = nullptr;
  }
  // Try to write the error message inside the error buffer present currently
  // OH NO, I WILL HAVE TO IMPLEMENT A SPECIAL PURPOSE BYTE-POOL ALSO FOR THIS TO RUN EFFICIENTLY

  // For error messages, you cannot reuse 'being used' buffers
  // First non-zero item denotes the 'size' of the following entry
  Error_Buffer_Unit_Slice msg_pool = global_error_pool_impl.msg_pool;
  for_slice(msg_pool, i){
     
  }

}

#endif // ERRORS_H_IMPL
#endif // ERRORS_H
