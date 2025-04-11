#include <stdio.h>
#include "tensor.h"
#include "tester.h"
#include "baseops.h"
#include "onstack.h"
#include "somearith.h"
#include "viewops.h"

int main(int argc, const const char* argv[]){
  TestCase cases[] = {
    {.entry_fxn = base_run, .test_name = "baseops"},
    {.entry_fxn = stack_run, .test_name = "onstack"},
    {.entry_fxn = arith_run, .test_name = "somearith"},
    {.entry_fxn = viewops_run, .test_name = "viewops"},
  };
  const size_t test_count = _countof(cases);
  return run_test(cases, _countof(cases),
		  "test_outs", "build/tests",
		  argc, argv);
}
