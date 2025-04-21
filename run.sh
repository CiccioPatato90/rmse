#!/bin/bash

# Default values
BUILD=false
PARAM=""

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -b)
      BUILD=true
      shift
      ;;
    *)
      PARAM="$1"
      shift
      ;;
  esac
done

# Check if parameter is provided
if [ -z "$PARAM" ]; then
  echo "Error: Parameter is required"
  echo "Usage: $0 [-b] <parameter>"
  echo "  -b: Build the project before running"
  echo "  <parameter>: Parameter to pass to batsim and gantt.py"
  exit 1
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

# Run batsim with the provided parameter
echo "Running batsim with parameter: $PARAM"
batsim -l "./build/lib${PARAM}.so" 0 '' -p assets/4machine.xml -w assets/non_contigous.json
#batsim -l "./build/lib${PARAM}.so" 0 '' -p assets/generated/machines/machines_5.xml -w assets/generated/gen.json
if [ $? -ne 0 ]; then
  echo "Batsim failed, exiting"
  exit 1
fi

# Run gantt.py with the provided parameter
echo "Running gantt.py with parameter: $PARAM"
python3 ./gantt.py "$PARAM"
if [ $? -ne 0 ]; then
  echo "Gantt.py failed, exiting"
  exit 1
fi

# Run analyze_scheduler_performance.py with the provided parameter
echo "Running analyze_scheduler_performance.py with parameter: $PARAM"
python3 ./scripts/analyze_scheduler_performance.py "$PARAM"
if [ $? -ne 0 ]; then
  echo "Analyze_scheduler_performance.py failed, exiting"
  exit 1
fi

echo "All operations completed successfully" 