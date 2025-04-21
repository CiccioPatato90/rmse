# BATSIM Project - M1 CSA 2024/2025

## Overview
This project implements various backfilling algorithms for job scheduling in BATSIM, inspired by Fernando Mendonça's PhD manuscript (Chapters 4 and 5). The project provides a framework for comparing different scheduling strategies and analyzing their performance.

## Prerequisites
This project uses Nix for dependency management and environment setup. Before using the project, you need to:

1. Install Nix package manager (if not already installed)
2. Enter the development environment:
   ```bash
   nix develop
   ```

This will set up all the necessary dependencies and tools required to build and run the project.

## Implemented Algorithms
Located in the `/src` directory, the project implements the following scheduling algorithms:

1. **Basic Backfilling (Conservative Backfilling)**
   - Standard implementation of conservative backfilling strategy
   - Prioritizes jobs in FIFO order
   - Allows smaller jobs to backfill if they don't delay the first job

2. **Best Effort Contiguous Backfilling**
   - Attempts to assign contiguous resources to tasks
   - Ensures tasks run on sequential resources (e.g., cannot run on resources with IDs 1 and 3)
   - Falls back to Basic scheduling if contiguous allocation is not possible

3. **Force Contiguous Backfilling**
   - Enforces contiguous resource allocation for all tasks
   - More strict version of contiguous backfilling
   - Rejects jobs if contiguous allocation is not possible

4. **First-Come-First-Served (FCFS)**
   - Simple non-backfilling scheduler
   - Executes jobs in strict submission order
   - No preemption or backfilling

5. **Execute-One-by-One**
   - Executes exactly one job at a time
   - Waits for the current job to complete before starting the next
   - Simplest possible scheduling strategy

## Usage

### Running Simulations
The project includes two scripts for running simulations:

#### Original Run Script
```bash
./run.sh <algorithm_name> [-b]
```
Parameters:
- `algorithm_name`: Name of the algorithm to run
- `-b`: Optional flag to build the algorithm source before execution

#### Enhanced Run Script (V2)
```bash
./runV2.sh [-b] [-j <num_jobs>] [-m <num_machines>] [--all] <algorithm_name>
```
Parameters:
- `-b`: Optional flag to build the algorithm source before execution
- `-j <num_jobs>`: Number of jobs to generate (default: 1000)
- `-m <num_machines>`: Number of machines to use (default: 5)
- `--all`: Run all algorithms with the same configuration
- `algorithm_name`: Name of the algorithm to run (required unless --all is used)

Examples:
```bash
# Run a single algorithm with default settings
./runV2 basic

# Run all algorithms with custom jobs and machines
./runV2 -j 2000 -m 10 --all

# Run a single algorithm with custom configuration
./runV2 -j 500 -m 8 basic
```

### Output
Results are stored in the following directory structure:

#### Original Run Script
Results are stored in `res/<algorithm_name>/` directory

#### Enhanced Run Script (V2)
Results are stored in `res/<algorithm_name>_<num_jobs>_<num_machines>/` directory

Each output directory contains:

1. **Summary File** (`.txt`)
   - Human-readable format of `jobs.csv` and `schedule.csv`
   - Basic performance analysis

2. **Waiting Time Distribution**
   - Distribution of job waiting times
   - Shows frequency of different waiting durations

3. **Waiting Time vs Submission Time**
   - Temporal analysis of waiting times
   - Helps understand scheduling patterns

4. **Gantt Chart** (`gantt.png`)
   - Visual representation of the schedule
   - Generated using Evalys

## Analysis Scripts

### Performance Analysis
The scheduler performance analysis script (`scripts/analyze_scheduler_performance.py`) generates comprehensive metrics for each algorithm:

### Makespan Analysis
The makespan analysis script (`scripts/plot_makespan.py`) compares the makespan performance of different algorithms:

```bash
python3 scripts/plot_makespan.py
```

This script:
- Reads makespan data from `res/makespan/` directory
- Creates comparison plots between algorithms
- Saves plots to `res/plots/` directory
- Outputs performance difference statistics

### Backfill Analysis
The backfill analysis script (`scripts/backfill.py`) analyzes the backfill behavior of different algorithms:

```bash
python3 scripts/backfill.py
```

This script:
- Reads backfill data from `res/backfill/` directory
- Creates probability density function plots for contiguous backfills
- Saves plots to `res/plots/` directory
- Provides visual comparison of backfill strategies

## Performance Metrics

The scheduler performance analysis script (`scripts/analyze_scheduler_performance.py`) generates comprehensive metrics for each algorithm:

### Backfill Statistics
- **Total Backfills**: The total number of backfills performed by the scheduler
- **Contiguous Backfills**: The number of backfills where the allocated resources are contiguous
- **Basic Backfills**: The number of backfills where the allocated resources are non-contiguous

### Resource Utilization
- **Average Resource Utilization**: Percentage of resources used over time
- **Peak Resource Utilization**: Maximum percentage of resources used at any point

### Job Metrics
- **Average Waiting Time**: Mean time jobs spend in the queue
- **Median Waiting Time**: Median time jobs spend in the queue
- **Maximum Waiting Time**: Longest time a job spent in the queue
- **Average Response Time**: Mean time from submission to completion
- **Job Completion Rate**: Percentage of submitted jobs that complete successfully

### Algorithm-Specific Metrics
- **Contiguity Rate**: Percentage of jobs allocated contiguous resources (for contiguous algorithms)
- **Rejection Rate**: Percentage of jobs rejected due to resource constraints

These metrics are stored in the root directory with the algorithm name, for example:
- `basic_backfill_stats.txt`
- `easy_backfill_stats.txt`
- `fcfs_stats.txt`
- `exec1by1_stats.txt`

The backfill statistics are also logged during the simulation and can be found in the console output and in the performance summary text file.

## Author
Francesco Pace Napoleone

## References
- Fernando Mendonça's PhD manuscript (Chapters 4 and 5)


