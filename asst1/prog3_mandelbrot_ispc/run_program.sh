#/bin/bash

# Function to check if a package is installed
is_package_installed() {
    dpkg -l | grep -qw "$1"
}

# Name of the package
PACKAGE="imagemagick-6.q16"


# Check if the package is installed
if ! is_package_installed "$PACKAGE"; then
    echo "$PACKAGE is not installed. Installing..."
    sudo apt-get update
    sudo apt-get install -y "$PACKAGE"
else
    echo "$PACKAGE is already installed."
fi

# make cpp
make

# Default params
MIN_LOOPS=1
DEFAULT_LOOPS=8
MAX_LOOPS=100
MIN_VIEW_INDEX=1
MAX_VIEW_INDEX=2

NUM_LOOPS=8
VIEW_INDEX=1
DIR="results/sol"
SPEED_RESULTS_NAME="speed_results_task_ispc.txt"


# Parse command-line options
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--dir)
            DIR="$2"
            # DIR cannot be empty
            if [[ -z "$DIR" ]]; then
                echo "Error: Directory cannot be empty"
                return 1
            fi
            shift 2
        ;;
        -m|--max-loops)
            NUM_LOOPS="$2"
            # Check if NUM_LOOPS 
            if ! [[ "$NUM_LOOPS" =~ ^[0-9]+$ ]] || [ "$NUM_LOOPS" -lt $MIN_LOOPS ] || [ "$NUM_LOOPS" -gt $MAX_LOOPS ]; then
                echo "Error: NUM_LOOPS must be a number between $MIN_LOOPS and $MAX_LOOPS."
                return 1
            fi
            shift 2
        ;;
        -v|--view-index)
            VIEW_INDEX="$2"
            # Check if VIEW_INDEX 
            if ! [[ "$VIEW_INDEX" =~ ^[1-9]+$ ]] || [ "$VIEW_INDEX" -lt $MIN_VIEW_INDEX ] || [ "$VIEW_INDEX" -gt $MAX_VIEW_INDEX ]; then
                echo "Error: VIEW_INDEX must be a number between $MIN_VIEW_INDEX and $MAX_VIEW_INDEX."
                return 1
            fi
            shift 2
        ;;
        *)  
            echo "Unknown option: $1"
            return 1
        ;;
    esac
done


# Check if the directory exists
if [ ! -d "$DIR" ]; then
    # Directory does not exist, so create it
    mkdir -p "$DIR"
    echo "Directory '$DIR' created."
fi

if [ -e "${DIR}/$SPEED_RESULTS_NAME" ]; then
    rm "${DIR}/$SPEED_RESULTS_NAME"
fi
touch "${DIR}/$SPEED_RESULTS_NAME"


echo "NUM_THREADS=$NUM_THREADS"
echo "NUM_LOOPS=$NUM_LOOPS"
echo "DIR=$DIR"

# Run the Mandelbrot program and process the results
for ((i=$MIN_LOOPS; i<=NUM_LOOPS; i++)); do
    echo "--------------";
    echo "Number of tasks: $i "
    echo "Running ./mandelbrot -t $i -d $DIR -v $VIEW_INDEX"
    ./mandelbrot_ispc -t $i -d $DIR -v $VIEW_INDEX
    if [[$i = $MIN_LOOPS]]; then 
        convert mandelbrot-task-ispc.ppm $DIR/mandelbrot-task-ispc-$i.png 
    fi
done


/home/ubuntu/miniforge3/envs/thienlu/bin/python3 ./py/viz_speed.py -d $DIR

##@@ Sample commands
# Run with specific number of threads: source run_program.sh -d results-opt2 -n 16 -v 2
# Run with loop numbers of threads: source run_program.sh -d results-opt2 -m 16 -v 2
