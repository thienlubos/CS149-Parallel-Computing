#!/bin/bash

# Default values
NUM_THREADS=16
TEST_NAME="super_light_async"

# Parse command line arguments

# Parse command-line options
while [[ $# -gt 0 ]]; do
    case $1 in
        -n|--num-threads)
            NUM_THREADS="$2"
            shift 2
        ;;
        -t|--test-name)
            TEST_NAME="$2"
            shift 2
        ;;
        *)  
            echo "Unknown option: $1"
            return 1
        ;;
    esac
done

echo "Running test $TEST_NAME with $NUM_THREADS threads"

# Build the project
make

# Run the tasks
./runtasks -n $NUM_THREADS $TEST_NAME
