#!/usr/bin/env bash

root_dir=$(dirname "$0")
cd $root_dir

if [ -d ./build ]; then
    echo "Build directory exists"
else
    echo "Build directory doesnot exist, creating... "
    mkdir ./build
fi

if [ -d ./build/tests ]; then
    echo "Tests directory exists"
else
    echo "Tests directory doesnot exist, creating... "
    mkdir ./build/tests
fi


# Check if UTILS_PATH is empty or not set
if [[ -z "$UTILS_PATH" ]]; then
    # Set UTILS_PATH to the default path
    UTILS_PATH="../c-utils/"
fi

# Check if the directory exists
if [[ ! -d "$UTILS_PATH" ]]; then
    echo "Utilities directory not found at "$UTILS_PATH
    echo "Clone from https://github.com/bipul018/c-utils"
    echo "  and paste it in the parent directory"
    echo "  or set 'UTILS_PATH' environment variable correctly."
    exit 1  # Exit with an error code
fi

# Find the compiler
if [[ -z "$CC" ]]; then
    if command -v gcc &> /dev/null; then
	CC='gcc'
    elif command -v clang &> /dev/null; then
	CC='clang'
    else
	echo "Neither gcc nor clang is installed. Please install one of them."
	exit 1
    fi
fi

echo "Compiling core object files using $CC ..."
$CC $CFLAGS -c -I$UTILS_PATH -I$(pwd)/src/ $(pwd)/src/tensor.c -o $(pwd)/build/tensor.obj
echo "Compiling the tests..."
$CC $CFLAGS -I$UTILS_PATH -I$(pwd)/src/ $(pwd)/build/tensor.obj $(pwd)/src/tests/run.c -o $(pwd)/build/run_tests
echo "Compiled!"

echo_cmd="echo"
for arg in "$@"; do
    if [[ "$arg" == "noecho" ]]; then
	echo_cmd=""
    fi
done

# Optionally test all if commanded
for arg in "$@"; do
    if [[ "$arg" =~ ^record: ]]; then
	case_name=${arg#record:}

	echo "Recording test case: $case_name"
	./build/run_tests $case_name $echo_cmd record
    fi	
    if [[ "$arg" =~ ^test: ]]; then
	case_name=${arg#test:}

	echo "Running test case: $case_name"
	./build/run_tests $case_name $echo_cmd
    fi
    if [[ "$arg" =~ ^example: ]]; then
	example_name=${arg#example:}

	echo "Building and running example from: $example_name.c"

	$CC $CFLAGS -I$UTILS_PATH -I$(pwd)/src/ $(pwd)/build/tensor.obj $(pwd)/src/example/"$example_name".c -o $(pwd)/build/ex_"$example_name"
        $(pwd)/build/ex_"$example_name"
    fi
done
