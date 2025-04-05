if [ -d ./build ]; then
    echo "Build directory exists"
else
    echo "Build directory doesnot exist, creating... "
    mkdir ./build
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
   
