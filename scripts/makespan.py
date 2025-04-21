#!/usr/bin/env python3
import os
import sys
import subprocess
import csv
import time
import random
import argparse
import signal

# Import the job generation function from generate_jobs.py
sys.path.append(os.path.dirname(os.path.abspath(__file__)))
from generate_jobs import generate_jobs
from generate_machines import generate_machines
from analyze_scheduler_performance import extract_backfill_stats

def init_output_files(algorithms, num_jobs, num_machines):
    for algorithm in algorithms:        
        # Create output file for this algorithm
        output_file_makespan = f"res/makespan/{algorithm}_temp.txt"
        output_file_backfill = f"res/backfill/{algorithm}_temp.txt"
        # Clear the file if it exists
        with open(output_file_makespan, 'w') as f:
            f.write(f"# Makespan values for {algorithm} algorithm\n")
            f.write(f"# Number of jobs: {num_jobs}, Number of machines: {num_machines}\n")
            f.write(f"# Format: simulation_number, makespan\n")
        with open(output_file_backfill, 'w') as f:
            f.write(f"# Backfill values for {algorithm} algorithm\n")
            f.write(f"# Number of jobs: {num_jobs}, Number of machines: {num_machines}\n")
            f.write(f"# Format: simulation_number, total_backfills, contiguous_backfills, non_contiguous_backfills\n")
    return output_file_makespan, output_file_backfill

def run_simulation(algorithm, num_machines):
    """Run a single simulation with the specified algorithm and parameters."""

    job_file = f"assets/generated/gen.json"
    algo_path = f"./build/lib{algorithm}.so"
    # machines_file = f"assets/generated/machines/machines_{num_machines}.xml"
    machines_file = f"assets/test/machines_5.xml"
    
    # Prepare the command as a list of arguments
    cmd = ["batsim", "-l", algo_path, "0", "", "-p", machines_file, "-w", job_file]
    
    print(f"Running simulation: {' '.join(cmd)}")
    
    try:
        # Run the command and capture output using subprocess.run
        # This ensures proper synchronization and error handling
        result = subprocess.run(cmd, check=True, capture_output=True, text=True)
        print(f"Command output: {result.stdout}")
    except subprocess.CalledProcessError as e:
        print(f"Command failed with exit code {e.returncode}")
        print(f"Error output: {e.stderr}")
        return None
    except Exception as e:
        print(f"Exception occurred: {e}")
        return None
    
    # Extract makespan from schedule.csv
    schedule_file = os.path.join("out/schedule.csv")
    if not os.path.exists(schedule_file):
        print(f"Error: Schedule file not found at {schedule_file}")
        #return None
    
    try:
        with open(schedule_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                print("row at makespan: ", row['makespan'])
                return float(row['makespan'])
    except Exception as e:
        print(f"Error reading schedule file: {e}")
        return None
    
    return None

def main():
    parser = argparse.ArgumentParser(description='Run multiple simulations with different algorithms')
    parser.add_argument('--num-sims', type=int, default=256, help='Number of simulations to run (default: 2)')
    parser.add_argument('--num-jobs', type=int, default=128, help='Number of jobs per simulation (default: 40)')
    parser.add_argument('--num-machines', type=int, default=5, help='Number of machines (default: 5)')
    parser.add_argument('--algorithms', nargs='+', default=['basic', 'best_cont', 'force_cont'], 
                        help='Algorithms to run (default: basic best_cont force_cont)')
    # parser.add_argument('--algorithms', nargs='+', default=['basic'], help='Algorithms to run (default: basic best_cont force_cont)')
    
    args = parser.parse_args()

    init_output_files(args.algorithms, args.num_jobs, args.num_machines)
    
    for i in range(args.num_sims):
        print(f"Running simulation {i+1}/{args.num_sims}...")
        generate_jobs(args.num_jobs, args.num_machines, "gen.json")
        generate_machines(args.num_machines, "assets/generated/machines") 
        for algorithm in args.algorithms:
            output_file_makespan = f"res/makespan/{algorithm}_temp.txt"
            # output_file_backfill = f"res/backfill/{algorithm}_temp.txt"

            print(f"Running {algorithm}...")

            makespan = run_simulation(algorithm, args.num_machines)
            # backfill = extract_backfill_stats(f"{algorithm}_log.txt")

            print(f"Makespan: {makespan}")
            # print(f"Backfill: {backfill}")

            if makespan is not None:
                # Append the makespan to the output file
                with open(output_file_makespan, 'a') as f:
                    f.write(f"{i+1} {makespan}\n")
            else:
                print(f"  Error: Could not extract makespan")
                # Add a placeholder for failed simulations
                with open(output_file_makespan, 'a') as f:
                    f.write(f"{i+1} FAILED\n")

            # if backfill is not None:
            #     # Append the makespan to the output file
            #     with open(output_file_backfill, 'a') as f:
            #         f.write(f"{i+1} {backfill}\n")
            # else:
            #     print(f"  Error: Could not extract backfill")
            #     # Add a placeholder for failed simulations
            #     with open(output_file_backfill, 'a') as f:
            #         f.write(f"{i+1} FAILED\n")

    print("\nAll simulations completed!")
    

if __name__ == "__main__":
    main() 