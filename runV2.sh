#!/bin/bash

# Default values
BUILD=false
PARAM=""
NUM_JOBS=10  # Default number of jobs
NUM_MACHINES=5  # Default number of machines
RUN_ALL=false

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -b)
      BUILD=true
      shift
      ;;
    -j)
      NUM_JOBS="$2"
      shift 2
      ;;
    -m)
      NUM_MACHINES="$2"
      shift 2
      ;;
    --all)
      RUN_ALL=true
      shift
      ;;
    *)
      if [ "$1" != "--all" ]; then
        PARAM="$1"
      fi
      shift
      ;;
  esac
done

# Check if parameter is provided (unless --all is used)
if [ -z "$PARAM" ] && [ "$RUN_ALL" = false ]; then
  echo "Error: Parameter is required unless --all is specified"
  echo "Usage: $0 [-b] [-j <num_jobs>] [-m <num_machines>] [--all] <parameter>"
  echo "  -b: Build the project before running"
  echo "  -j <num_jobs>: Number of jobs to generate (default: 1000)"
  echo "  -m <num_machines>: Number of machines to use (default: 5)"
  echo "  --all: Run all algorithms with the same number of jobs"
  echo "  <parameter>: Parameter to pass to batsim and gantt.py"
  exit 1
fi

# Generate jobs if -j is specified
if [ ! -z "$NUM_JOBS" ]; then
  echo "Generating $NUM_JOBS jobs..."
  python3 ./scripts/generate_jobs.py "$NUM_JOBS" "$NUM_MACHINES" "gen.json"
  if [ $? -ne 0 ]; then
    echo "Job generation failed, exiting"
    exit 1
  fi
  echo "Jobs generated successfully"
fi

# Build the project if -b flag is provided
if [ "$BUILD" = true ]; then
  echo "Building project..."
  ninja -C build
  if [ $? -ne 0 ]; then
    echo "Build failed, exiting"
    exit 1
  fi
  echo "Build completed successfully"
fi

# Function to run a single algorithm
run_algorithm() {
  local algo=$1
  local output_dir="res/${algo}_${NUM_JOBS}_${NUM_MACHINES}"
  echo "Running algorithm: $algo"
  
  # Run batsim with the provided parameter
  echo "Running batsim with parameter: $algo"
  batsim -l "./build/lib${algo}.so" 0 '' -p "assets/test/machines_${NUM_MACHINES}.xml" -w assets/generated/gen.json
  if [ $? -ne 0 ]; then
    echo "Batsim failed for $algo, continuing with next algorithm..."
    return 1
  fi

  # Run gantt.py with the provided parameter
  echo "Running gantt.py with parameter: $algo"
  python3 ./gantt.py "$algo" "$output_dir"
  if [ $? -ne 0 ]; then
    echo "Gantt.py failed for $algo, continuing with next algorithm..."
    return 1
  fi

  # Run analyze_scheduler_performance.py with the provided parameter
  echo "Running analyze_scheduler_performance.py with parameter: $algo"
  python3 ./scripts/analyze_scheduler_performance.py "$algo" "$output_dir"
  if [ $? -ne 0 ]; then
    echo "Analyze_scheduler_performance.py failed for $algo, continuing with next algorithm..."
    return 1
  fi

  echo "All operations completed successfully for $algo"
  return 0
}

# Run either all algorithms or a single one
if [ "$RUN_ALL" = true ]; then
  algorithms=("basic" "best_cont" "force_cont" "easy_backfill")
  for algo in "${algorithms[@]}"; do
    run_algorithm "$algo"
  done
else
  run_algorithm "$PARAM"
fi

echo "All operations completed" 