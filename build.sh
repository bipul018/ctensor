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
    echo "Utilities directory not found."
    echo "Clone from https://github.com/bipul018/c-utils"
    echo "  and paste it in the parent directory"
    echo "  or set 'UTILS_PATH' environment variable correctly."
    exit 1  # Exit with an error code
fi

echo "Compiling core object files..."
cc -c -I$UTILS_PATH tensor.c -o ./build/tensor.obj
echo "Compiling examples file..."
cc -I$UTILS_PATH ./build/tensor.obj examples.c -o ./build/examples
echo "Compiled!"
   
