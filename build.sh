#!/usr/bin/env bash

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
    UTILS_PATH="$(pwd)/c-utils/"
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
if command -v gcc &> /dev/null; then
    CC='gcc'
elif command -v clang &> /dev/null; then
    CC='clang'
else
    echo "Neither gcc nor clang is installed. Please install one of them."
    exit 1
fi

echo "Compiling core object files using $CC ..."
$CC -c -I$UTILS_PATH tensor.c -o ./build/tensor.obj
echo "Compiling examples file..."
$CC -I$UTILS_PATH ./build/tensor.obj examples.c -o ./build/examples
echo "Compiled!"

# Optionally test all if commanded
for arg in "$@"; do
    if [[ "$arg" =~ ^record: ]]; then
	case_name=${arg#record:}

	echo "Recording test case: $case_name"
	./build/examples $case_name echo record
    fi	
    if [[ "$arg" =~ ^test: ]]; then
	case_name=${arg#test:}

	echo "Running test case: $case_name"
	./build/examples $case_name echo
    fi
done
