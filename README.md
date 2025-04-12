# BATSIM Project - M1 CSA 2024/2025

## Overview
This project implements various backfilling algorithms for job scheduling in BATSIM, inspired by Fernando Mendonça's PhD manuscript (Chapters 4 and 5).

## Implemented Algorithms
Located in the `/src` directory, the project implements three scheduling algorithms:

1. **Basic Backfilling (Conservative Backfilling)**
   - Standard implementation of conservative backfilling strategy

2. **Best Effort Contiguous Backfilling**
   - Attempts to assign contiguous resources to tasks
   - Ensures tasks run on sequential resources (e.g., cannot run on resources with IDs 1 and 3)
   - Falls back to Basic scheduling if contiguous allocation is not possible

3. **Force Contiguous Backfilling**
   - Enforces contiguous resource allocation for all tasks
   - More strict version of contiguous backfilling

## Usage

### Running Simulations
The project includes two scripts for running simulations:

#### Original Run Script
```bash
./run <algorithm_name> [-b]
```
Parameters:
- `algorithm_name`: Name of the algorithm to run
- `-b`: Optional flag to build the algorithm source before execution

#### Enhanced Run Script (V2)
```bash
./runV2 [-b] [-j <num_jobs>] [-m <num_machines>] [--all] <algorithm_name>
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

## Author
Francesco Pace Napoleone

## References
- Fernando Mendonça's PhD manuscript (Chapters 4 and 5)


