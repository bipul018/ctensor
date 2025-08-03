#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

// A testing mechanism

// entry fxn , test name, test location, testing or
//       recording mode determined by cmd args or compiler defines

typedef struct TestCase TestCase;
struct TestCase {
  int (*entry_fxn)(int argc, const char* argv[]);
  const char* test_name;
};

static void copy_file_stream(FILE* destFile, FILE* sourceFile){
  fseek(sourceFile, 0, SEEK_SET);
  char ch;
  while ((ch = fgetc(sourceFile)) != EOF) {
    fputc(ch, destFile);
  }
}

#define err_and_ret(exit_code, ...)		\
  do{						\
    fprintf(stderr,				\
	    "Error: %s:%d: ",			\
	    __FILE__, __LINE__);		\
    fprintf(stderr, __VA_ARGS__);		\
    return (exit_code);				\
  }while(0)

static bool compare_files(FILE *file1, FILE *file2) {
  fseek(file1, 0, SEEK_SET);
  fseek(file2, 0, SEEK_SET);
  char ch1, ch2;
  int pos = 0, line = 1;

  while(true){
    ch2 = fgetc(file2);
    ch1 = fgetc(file1);
    if((ch2 == EOF) || (ch1 == EOF)) break;
    pos++;
    if (ch1 == '\n' && ch2 == '\n') {
      line++;
      pos = 0;
    }
    if (ch1 != ch2) {
      printf("Line Number: %d, Error Position: %d\n", line, pos);
      // Also print some characters:
      const int view = 30;
      for(int i = 0; i < 20; ++i) printf("+");
      printf("\n");
      for(int i = 0; i < view; ++i){
	printf("%c", ch1);
	ch1 = fgetc(file1);
	if(ch1 == EOF) break;
      }
      printf("\n");
      for(int i = 0; i < 20; ++i) printf("+");
      printf("\n");
      for(int i = 0; i < 20; ++i) printf("-");
      printf("\n");
      for(int i = 0; i < view; ++i){
	printf("%c", ch2);
	ch2 = fgetc(file2);
	if(ch2 == EOF) break;
      }
      printf("\n");
      for(int i = 0; i < 20; ++i) printf("-");
      printf("\n");
      return false;
    }
  }

  if (ch1 != EOF) {
    printf("File 1 has more content.\n");
    return false;
  } else if (ch2 != EOF) {
    printf("File 2 has more content.\n");
    return false;
  }
  return true;
}

static char* concat_strings(const char* strings[], size_t count){
  // count lengths
  size_t total_len = 1;
  for(size_t i = 0; i < count; ++i) total_len += strlen(strings[i]);
  // allocate

  char* string = malloc(total_len);
  if(!string) return string;
  string[0] = 0;

  // strcat
  for(size_t i = 0; i < count; ++i)
    (void)strcat(string, strings[i]);

  return string;
}

// TODO:: Maybe dump the error messages with a special 'prefix' even though now they are in same file

static int run_test(TestCase test_cases[], size_t test_case_count, const char* test_dir, const char* temp_test_dir, int argc, const char* argv[]){
  // Take some args

  // First is the test case to run

  // Then are is the flag 'record' that enables recording mode
  // Or the flag 'echo' that enables echoing results to the console

  // Then ignore upto the first '--'

  // Then forward that to the test case

  if(argc < 2){
    err_and_ret(-1, "Error: usage : <testcase> [record] -- <test case cmd args>\n");
  }

  const char* test_case = argv[1];

  // Find the test case to run
  int (*test_case_fxn)(int argc, const char* argv[]) = NULL;
  for(size_t i = 0; i < test_case_count; ++i){
    if(strcmp(test_case, test_cases[i].test_name) == 0) {
      test_case_fxn = test_cases[i].entry_fxn;
      break;
    }
  }
  if(test_case_fxn == NULL){
    err_and_ret(-1, "Error: No test case registered for %s\n", test_case);
  }

  // Parse other args
  bool also_echo = false;
  bool record_mode = false;
  int test_arg_inx = 2;
  for(;test_arg_inx < argc; ++test_arg_inx){
    if(strcmp(argv[test_arg_inx], "--") == 0){
      test_arg_inx++;
      break;
    }
    if(strcmp(argv[test_arg_inx], "record") == 0){
      record_mode = true;
    }
    if(strcmp(argv[test_arg_inx], "echo") == 0){
      also_echo = true;
    }
  }

  const char** test_argv = malloc(argc - test_arg_inx + 1);
  if(test_argv == NULL){
    err_and_ret(-1, "Error: Couldnot allocate memory for test %s\n", test_case);
  }

  test_argv[0] = test_case;
  const int test_argc = 1 + argc - test_arg_inx;

  for(int i = 1; i < test_argc; ++i){
    test_argv[i] = argv[i+test_arg_inx-1];
  }

  // Combine the temporary test case results file directory with names
  // Last element is to be edited by self and used
  const char* temp_file_prefix[] = {
    temp_test_dir, "/", test_case, ""
  };
  
  temp_file_prefix[3] = "_out.txt";
  char* out_file_name =
    concat_strings(temp_file_prefix, 4);

  //temp_file_prefix[3] = "_stderr.txt";
  //char* err_file_name =
  //  concat_strings(temp_file_prefix, 4);

  if(!out_file_name /* || !err_file_name */){
    err_and_ret(-1, "Error: Couldnot allocate memory for test %s\n", test_case);
  }
  
  FILE* out_file = fopen(out_file_name, "w");
  if(!out_file) {
    err_and_ret(-1, "Error: Couldnot open file `%s` for writing test cases\n", out_file_name);
  }
  //FILE* err_file = fopen(err_file_name, "w");
  //if(!err_file) {
  //  err_and_ret(-1, "Error: Couldnot open file `%s` for writing test cases\n", err_file_name);
  //}

  FILE* old_stdout = stdout;
  FILE* old_stderr = stderr;  

  stdout = out_file;
  stderr = out_file; //err_file;

  // Disable output file buffering
  //   (This was a very very frustating bug)
  //   (I couldnot even do traditional printf debugging)
  //   (and gdb debugging sucks *ss)
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0); 

  // First run the test case and write results on the file
  int code = test_case_fxn(test_argc, test_argv);

  stdout = old_stdout;
  stderr = old_stderr;

  // Reopen file for reading
  fclose(out_file);
  //fclose(err_file);
  out_file = fopen(out_file_name, "r");
  if(!out_file) {
    err_and_ret(-1, "Error: Couldnot open file `%s` for reading for test cases\n", out_file_name);
  }
  //err_file = fopen(err_file_name, "r");
  //if(!err_file) {
  //  err_and_ret(-1, "Error: Couldnot open file `%s` for reading for test cases\n", err_file_name);
  //}

  // If said to echo, echo
  if(also_echo){
    copy_file_stream(stdout, out_file);
    //copy_file_stream(stderr, err_file);
  }

  if(code != 0){
    printf("The test case %s exited with code %d\n", test_case, code);
  }

  // Setup file names to the actual test file
  temp_file_prefix[0] = test_dir;

  temp_file_prefix[3] = "_out.txt";
  char* actual_out_file_name =
    concat_strings(temp_file_prefix, 4);

  //temp_file_prefix[3] = "_stderr.txt";
  //  char* actual_err_file_name =
  //    concat_strings(temp_file_prefix, 4);

  // Now if they are in record mode, copy contents (or file) to new dsts
  FILE *actual_out_file;//, *actual_err_file;
  if(record_mode){
    actual_out_file = fopen(actual_out_file_name, "w");
    if(!actual_out_file) {
      err_and_ret(-1, "Error: Couldnot open file `%s` for writing test cases\n", actual_out_file_name);
    }
    //actual_err_file = fopen(actual_err_file_name, "w");
    //if(!actual_err_file) {
    //  err_and_ret(-1, "Error: Couldnot open file `%s` for writing test cases\n", actual_err_file_name);
      //}

    printf("Recording `%s`\n",
	   actual_out_file_name);//, actual_err_file_name);

    copy_file_stream(actual_out_file, out_file);
    //copy_file_stream(actual_err_file, err_file);
    
  }
  else{
    actual_out_file = fopen(actual_out_file_name, "r");
    if(!actual_out_file) {
      err_and_ret(-1, "Error: Couldnot open file `%s` for reading test cases\n", actual_out_file_name);
    }
    //actual_err_file = fopen(actual_err_file_name, "w");
    //if(!actual_err_file) {
    //  err_and_ret(-1, "Error: Couldnot open file `%s` for reading test cases\n", actual_err_file_name);
    //}

    // Now compare contents
    if(compare_files(actual_out_file, out_file)/*  && */
       /* compare_files(actual_err_file, err_file) */){
      printf("Test passed ✅\n");
    } else {
      printf("Test failed ❌\n");
    }
  }

  fclose(actual_out_file);
  //fclose(actual_err_file);

  fclose(out_file);
  //fclose(err_file);

  return 0;
}
