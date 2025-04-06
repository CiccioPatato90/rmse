#!/bin/bash

# Default values
BUILD=false
SCHEDULER="easy"
PLATFORM="assets/test/machines_5.xml"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
  case $1 in
    -b)
      BUILD=true
      shift
      ;;
    -s)
      SCHEDULER="$2"
      shift 2
      ;;
    -p)
      PLATFORM="$2"
      shift 2
      ;;
    *)
      echo "Unknown option: $1"
      echo "Usage: $0 [-b] [-s scheduler] [-p platform.xml]"
      echo "  -b: Build the project before running"
      echo "  -s: Specify scheduler (default: easy)"
      echo "  -p: Specify platform file (default: assets/test/machines_5.xml)"
      exit 1
      ;;
  esac
done

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

# Create output directory if it doesn't exist
mkdir -p output/backfill_comparison

# Define the workloads to run
WORKLOADS=(
  "backfill_test_plus10.json"
  "backfill_test_plus100.json"
  "backfill_test_plus1000.json"
  "backfill_test_plus10000.json"
)

# Run simulations for each workload
for WORKLOAD in "${WORKLOADS[@]}"; do
  echo "==================================================="
  echo "Running simulation with workload: $WORKLOAD"
  echo "==================================================="
  
  # Extract the base name of the workload file without extension
  WORKLOAD_BASE=$(basename "$WORKLOAD" .json)
  
  # Run batsim with the provided parameters
  echo "Running batsim with workload: $WORKLOAD, scheduler: $SCHEDULER, platform: $PLATFORM"
  batsim -l "./build/lib${SCHEDULER}.so" 0 '' -p $PLATFORM -w assets/generated/$WORKLOAD
  if [ $? -ne 0 ]; then
    echo "Batsim failed for $WORKLOAD, continuing with next workload"
    continue
  fi
  
  # Run gantt.py with the provided parameters
  echo "Running gantt.py with scheduler: $SCHEDULER"
  python3 ./gantt.py "$SCHEDULER"
  if [ $? -ne 0 ]; then
    echo "Gantt.py failed for $WORKLOAD, continuing with next workload"
    continue
  fi
  
  # Move the output files to the comparison directory with workload-specific names
  mv output/${SCHEDULER}.json output/backfill_comparison/${WORKLOAD_BASE}_${SCHEDULER}.json
  mv output/${SCHEDULER}.png output/backfill_comparison/${WORKLOAD_BASE}_${SCHEDULER}.png
  
  echo "Completed simulation for $WORKLOAD"
  echo "==================================================="
done

echo "All simulations completed successfully"
echo "Results are available in the output/backfill_comparison directory" 