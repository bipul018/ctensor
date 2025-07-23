#ifndef ERRORS_H
#define ERRORS_H

#ifdef ERRORS_H_SOLO_RUN_TEST
#define ERRORS_H_IMPL
#endif //ERRORS_H_SOLO_RUN_TEST

#define UTIL_INCLUDE_ALL
#include <util_headers.h>

typedef struct Error_Chain Error_Chain;
struct Error_Chain{
  // nullptr => end of the error chain,
  // -1 => a free error node (used for creating error pool)
  Error_Chain* prev;
  // A null terminated error message allocated just with this struct
  char msg[1];
};

// When making a new error chain node, prefer to rather erase the head and use it as tail
//   rather than returning error (It probably is better to report the latest error in the chain)
// Only if this is a fresh error, then you can return nullptr (or you probably can also assert that part)
void init_global_error_chain_pool(Alloc_Interface allocr, size_t count);
void free_global_error_chain_pool(Alloc_Interface allocr);
Error_Chain* new_error(Error_Chain* prev_err, const char* file, const char* func, int lineno, const char* fmt, ...);
// TODO:: Find out if this is even worth doing at all
void free_error(Error_Chain* err_chain);


#ifdef ERRORS_H_IMPL
#include<stdarg.h>
#include<stdio.h>
#include<stddef.h>

// The union that actually represents the underlying storage of error chains
typedef union Error_Pool_Unit Error_Pool_Unit;
union Error_Pool_Unit {
  Error_Chain chn;
  struct{
    Error_Pool_Unit* next;
    size_t sz;
  };
};
DEF_SLICE(Error_Pool_Unit);
static struct {
  Error_Pool_Unit_Slice pool;
  Error_Pool_Unit* free_units;
} global_error_pool = {0};

void init_global_error_chain_pool(Alloc_Interface allocr, size_t count){
  global_error_pool.pool = SLICE_ALLOC(allocr, Error_Pool_Unit, count);
  assert(global_error_pool.pool.data != nullptr);
  assert(global_error_pool.pool.count != 0);

  global_error_pool.free_units = global_error_pool.pool.data;
  global_error_pool.free_units->next = nullptr;
  global_error_pool.free_units->sz =  global_error_pool.pool.count;
}
void free_global_error_chain_pool(Alloc_Interface allocr){
  SLICE_FREE(allocr, global_error_pool.pool);
  global_error_pool.free_units = nullptr;
}

// function to write assuming its enough, returns leftover (if it was left)
static Error_Pool_Unit* write_err_v(Error_Pool_Unit* err, const char* file, const char* func, int lineno, const char* fmt, va_list args){
  Error_Pool_Unit nerr = {0};
  if(err != nullptr){
    nerr = *err;
  }
  const size_t max_sz = ((err == nullptr)?0:(sizeof(Error_Pool_Unit) * err->sz - offsetof(Error_Chain, msg)));
  char* dst = ((err==nullptr)?nullptr:err->chn.msg);
  const size_t sz1 = snprintf(dst, max_sz, "%s:%d:(%s): ",
		  (file == nullptr?"":file),lineno,(func == nullptr?"":func));
  const size_t sz2 = vsnprintf(dst + ((max_sz == 0)?0:sz1),
			       max_sz - ((max_sz == 0)?0:sz1),
			       fmt, args);  
  const size_t consumed = sz1 + sz2 + 1 + offsetof(Error_Chain, msg);

  const size_t unit_cnt = (consumed + sizeof(Error_Pool_Unit) - 1) / sizeof(Error_Pool_Unit);
  if(err != nullptr && unit_cnt < nerr.sz){
    err[unit_cnt] = nerr;
    err[unit_cnt].sz -= unit_cnt;
    return err + unit_cnt;
  }
  return nullptr;
}

// function to find out the no of nodes consumed
static size_t calc_no_units_v(const char* file, const char* func, int lineno, const char* fmt, va_list args){
  return (size_t)(write_err_v(nullptr, file, func, lineno, fmt, args) - (Error_Pool_Unit*)nullptr);
}

// Given an msg pool, get the >= matching if exists or the end (also removes it)
// Or returns the last entry

static Error_Pool_Unit* get_appropriate_node(Error_Pool_Unit** p_free_node, size_t units){
  assert(p_free_node != nullptr && (*p_free_node) != nullptr);

  while((*p_free_node)->next != nullptr && (*p_free_node)->sz < units){
    p_free_node = &(*p_free_node)->next;
  }
  Error_Pool_Unit* chosen = *p_free_node;

  // Just remove this node, splitting logic to be decided later
  *p_free_node = chosen->next;
  chosen->next = nullptr;
  
  return chosen;
}

// Given an msg pool, insert the new unit in ascending order
static void insert_asc_node(Error_Pool_Unit** p_free_node, Error_Pool_Unit* new_node){
  // First find the node whose size is greater
  assert(p_free_node != nullptr && new_node != nullptr);
  while(*p_free_node != nullptr && (*p_free_node)->sz < new_node->sz){
    p_free_node = &(*p_free_node)->next;
  }
  new_node->next = *p_free_node;
  *p_free_node = new_node;
}

/*
void free_error(Error_Chain* root){
  // First free the inner buffer
  
  // Then free the error chain itself

  
}
*/
void free_error(Error_Chain* err_chain){
  assert(("Not implemented yet!!!", false));
}
Error_Chain* new_error(Error_Chain* prev_err, const char* file, const char* func, int lineno, const char* fmt, ...){
  // First find out if any error is free
  if(global_error_pool.free_units == nullptr){
    // Try to reclaim from the top of prev_err
    // TODO:: Figure out a way for this, or a better assert message
    assert(prev_err != nullptr);
    Error_Chain** ptop = &prev_err;
    while((*ptop)->prev != nullptr) ptop = &(*ptop)->prev;
    Error_Chain* top = *ptop;
    *ptop = nullptr;
    // Reinsert this into the free_units
    free_error(top);
    // Now at least one should be available as the free error
  }
  // Now calculate the required size
  va_list args = {0};

  va_start(args, fmt);
  size_t node_num = calc_no_units_v(file, func, lineno, fmt, args);
  va_end(args);
  
  // Find one from the free list (which should have at least one)
  Error_Pool_Unit* node = get_appropriate_node(&global_error_pool.free_units, node_num);

  // Write on the error
  va_start(args, fmt);
  Error_Pool_Unit* left = write_err_v(node, file, func, lineno, fmt, args);
  va_end(args);

  // If there was more, reinsert the remaining part
  if(left != nullptr) insert_asc_node(&global_error_pool.free_units, left);

  // Return the error node
  return &node->chn;
  
  /*
  Error_Pool_Unit* perr = nullptr;
  for_slice(global_error_pool.pool, i){
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
  // TODO:: Handle no message case, for now just assert
  assert(("Havent thought of any case where you dont even have error pool!!!",
	  global_error_pool_impl.free_msgs != nullptr));

  // Now find the length of the destination string
  size_t msg_len = 0;
  msg_len += snprintf(nullptr, 0, "%s:%d(%s) ",
		      (file == nullptr?"":file),(func == nullptr?"":func),
		      lineno);
  va_list args={0};
  va_start(args, fmt);
  msg_len += vsnprintf(nullptr, 0, fmt, args);
  va_end(args);

  // Now find the node that allows this long output
  Error_Buffer_Unit* ch_buf = get_appropriate_bfr(&global_error_pool_impl.free_msgs, msg_len);

  // Try to split the node if possible
  size_t msg_node_count = (msg_len + sizeof(Error_Buffer_Unit)) / sizeof(Error_Buffer_Unit);
  // More/less, doesnt matter, just print with that size ??
  if(ch_buf->size > msg_node_count){
    // Truncate and insert
    Error_Buffer_Unit* posterior = ch_buf + msg_node_count;
    posterior->size = ch_buf->size - msg_node_count;
    posterior->next = nullptr;
    insert_asc_bfr(&global_error_pool_impl.free_msgs, posterior);
  }

  // Now sprintf that thing
  // TODO:: Find out if you need to care about the return value
  (void)snprintf((char*)ch_buf, msg_len, "%s:%d(%s) ",
		 (file == nullptr?"":file),(func == nullptr?"":func),
		 lineno);
  va_start(args, fmt);
  msg_len += vsnprintf((char*)ch_buf+strlen((char*)ch_buf),
		       msg_len - strlen((char*)ch_buf), fmt, args);
  va_end(args);

  perr->prev = prev_err;
  perr->err_msg = (const char*)ch_buf;
  return perr;
  */
  return nullptr;
}

#endif // ERRORS_H_IMPL


#ifdef ERRORS_H_SOLO_RUN_TEST
// gcc -DERRORS_H_KEEP_EXE -I$HOME/prgming/c-utils/ -xc errors.h -DERRORS_H_SOLO_RUN_TEST -o errors.out && ./errors.out
#include <stdio.h>

static size_t calc_no_units(const char* file, const char* func, int lineno, const char* fmt, ...){
  va_list args = {0};
  va_start(args, fmt);
  size_t ans = calc_no_units_v(file, func, lineno, fmt, args);
  va_end(args);
  return ans;
}
static Error_Pool_Unit* write_err(Error_Pool_Unit* err, const char* file, const char* func, int lineno, const char* fmt, ...){
  va_list args = {0};
  va_start(args, fmt);
  Error_Pool_Unit* ans = write_err_v(err, file, func, lineno, fmt, args);
  va_end(args);
  return ans;
}

int main(int argc, char* argv[]){

#ifndef ERRORS_H_KEEP_EXE
  // First delete self
  assert(argc >= 1);
  printf("Deleting self at %s...\n", argv[0]);
  remove(argv[0]);
#endif //ERRORS_H_KEEP_EXE

  Alloc_Interface allocr = gen_std_allocator();
  init_global_error_chain_pool(allocr, 2048);

  printf("%p, %zu, %p, %p, %zu!\n",
	 global_error_pool.pool.data,
	 global_error_pool.pool.count,
	 global_error_pool.free_units,
	 global_error_pool.free_units->next,
	 global_error_pool.free_units->sz);
  
  size_t sz = calc_no_units(__FILE__, __func__, __LINE__, "%s", "hi");

  printf("Sizeof error struct : %zu\n", sizeof(Error_Pool_Unit));
  printf("The required size is: %zu\n", sz);

  Error_Pool_Unit errs[3] = {0};
  errs->sz = _countof(errs);
  errs->next = (void*)0x1234; // Just a dummy value

  Error_Pool_Unit* nerr = write_err(errs, __FILE__, __func__, __LINE__, "%s", "hi");

  printf("The error is \n*************\n%s\n***********\n", errs->chn.msg);
  
  printf("The current error is %p, next error is %p, difference is %zu\n",
	 errs, nerr, (nerr-errs));
  printf("The usedup length is %zu\n",
	 1 + strlen(errs->chn.msg) + offsetof(Error_Chain, msg));
  if(nerr != nullptr) printf("The value in next error is: %p, %zu\n",
			     nerr->next, nerr->sz);



  Error_Chain* err = new_error(nullptr, __FILE__, __func__, __LINE__, "Maybe I can do things");
  printf("err is %p\n", err);
  if(err != nullptr){
    printf("Inside err is msg \n%s, prev is %p\n", err->msg, err->prev);
  }

  err = new_error(err, __FILE__, __func__, __LINE__, "Could do stuff, not an error");
  printf("err is %p\n", err);
  if(err != nullptr){
    printf("Inside err is msg \n%s, prev is %p\n", err->msg, err->prev);
  }
  
  free_global_error_chain_pool(allocr);
  return 0;
}


#endif //ERRORS_H_SOLO_RUN_TEST



#endif // ERRORS_H
