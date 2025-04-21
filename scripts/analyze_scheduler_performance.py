#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import os
import sys
import re
from pathlib import Path

def extract_backfill_stats(log_file):
    """
    Extract the final backfill statistics from a log file.
    
    Args:
        log_file: Path to the log file
    
    Returns:
        tuple: (total_backfills, contiguous_backfills, basic_backfills) or None if not found
    """
    if not os.path.exists(log_file):
        return None
    
    # Read the last line of the file that contains backfill statistics
    with open(log_file, 'r') as f:
        lines = f.readlines()
    
    # Find the last line with backfill statistics
    for line in reversed(lines):
        line = line.strip().split(' ')
        return int(line[0]), int(line[1]), int(line[2])

def analyze_scheduler_performance(algorithm_name, output_dir=None):
    """
    Analyze scheduler performance from jobs.csv and schedule.csv files.
    
    Args:
        algorithm_name: Name of the algorithm used for the simulation
        output_dir: Directory where to save the results (optional)
    """
    # Define input paths
    input_jobs_csv = "./out/jobs.csv"
    input_schedule_csv = "./out/schedule.csv"
    log_file = f"./out/{algorithm_name}_log.txt"
    
    # Set default output directory if not provided
    if output_dir is None:
        output_dir = f"res/{algorithm_name}"
    
    output_prefix = f"{output_dir}/{algorithm_name}"
    
    # Check if input files exist
    if not os.path.exists(input_jobs_csv):
        print(f"Error: Input file '{input_jobs_csv}' not found.")
        sys.exit(1)
    
    if not os.path.exists(input_schedule_csv):
        print(f"Error: Input file '{input_schedule_csv}' not found.")
        sys.exit(1)
    
    # Create output directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    # Load data
    jobs_df = pd.read_csv(input_jobs_csv)
    schedule_df = pd.read_csv(input_schedule_csv)
    
    # Extract backfill statistics from log file
    backfill_stats = extract_backfill_stats(log_file)
    if backfill_stats:
        total_backfills, contiguous_backfills, basic_backfills = backfill_stats
    else:
        total_backfills, contiguous_backfills, basic_backfills = 0, 0, 0
    
    # Extract basic statistics
    total_jobs = len(jobs_df)
    makespan = schedule_df['makespan'].iloc[0]
    mean_waiting_time = schedule_df['mean_waiting_time'].iloc[0]
    mean_turnaround_time = schedule_df['mean_turnaround_time'].iloc[0]
    mean_slowdown = schedule_df['mean_slowdown'].iloc[0]
    max_slowdown = schedule_df['max_slowdown'].iloc[0]
    
    # Print summary statistics
    print("\n=== SCHEDULER PERFORMANCE SUMMARY ===")
    print(f"Total Jobs: {total_jobs}")
    print(f"Makespan: {makespan:.2f} seconds")
    print(f"Mean Waiting Time: {mean_waiting_time:.2f} seconds")
    print(f"Mean Turnaround Time: {mean_turnaround_time:.2f} seconds")
    print(f"Mean Slowdown: {mean_slowdown:.2f}")
    print(f"Max Slowdown: {max_slowdown:.2f}")
    print(f"\nBackfill Statistics:")
    print(f"Total Backfills: {total_backfills}")
    print(f"Contiguous Backfills: {contiguous_backfills}")
    print(f"Basic Backfills: {basic_backfills}")
    
    # Resource utilization
    total_computing_time = schedule_df['time_computing'].iloc[0]
    total_idle_time = schedule_df['time_idle'].iloc[0]
    nb_machines = schedule_df['nb_computing_machines'].iloc[0]
    
    # Calculate resource utilization percentage
    total_time = makespan * nb_machines
    utilization = (total_computing_time / total_time) * 100
    
    print(f"\nResource Utilization: {utilization:.2f}%")
    
    # Analyze job characteristics
    print("\n=== JOB CHARACTERISTICS ===")
    print(f"Average Resources per Job: {jobs_df['requested_number_of_resources'].mean():.2f}")
    print(f"Average Execution Time: {jobs_df['execution_time'].mean():.2f} seconds")
    print(f"Average Waiting Time: {jobs_df['waiting_time'].mean():.2f} seconds")
    print(f"Average Stretch: {jobs_df['stretch'].mean():.2f}")
    
    # Resource distribution
    resource_counts = jobs_df['requested_number_of_resources'].value_counts().sort_index()
    print("\nResource Request Distribution:")
    for res, count in resource_counts.items():
        print(f"  {res} resources: {count} jobs ({count/total_jobs*100:.1f}%)")
    
    # Create visualizations
    
    # 1. Waiting Time Distribution
    plt.figure(figsize=(10, 6))
    plt.hist(jobs_df['waiting_time'], bins=20, alpha=0.7, color='skyblue')
    plt.title('Waiting Time Distribution')
    plt.xlabel('Waiting Time (seconds)')
    plt.ylabel('Number of Jobs')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.savefig(f"{output_prefix}_waiting_time_distribution.png")
    
    # 4. Waiting Time vs. Submission Time
    plt.figure(figsize=(10, 6))
    plt.scatter(jobs_df['submission_time'], jobs_df['waiting_time'], alpha=0.7, c='purple')
    plt.title('Waiting Time vs. Submission Time')
    plt.xlabel('Submission Time (seconds)')
    plt.ylabel('Waiting Time (seconds)')
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.savefig(f"{output_prefix}_waiting_vs_submission.png")
    
    # Save summary to a text file
    with open(f"{output_prefix}_performance_summary.txt", 'w') as f:
        f.write("=== SCHEDULER PERFORMANCE SUMMARY ===\n")
        f.write(f"Total Jobs: {total_jobs}\n")
        f.write(f"Makespan: {makespan:.2f} seconds\n")
        f.write(f"Mean Waiting Time: {mean_waiting_time:.2f} seconds\n")
        f.write(f"Mean Turnaround Time: {mean_turnaround_time:.2f} seconds\n")
        f.write(f"Mean Slowdown: {mean_slowdown:.2f}\n")
        f.write(f"Max Slowdown: {max_slowdown:.2f}\n")
        f.write(f"\nBackfill Statistics:\n")
        f.write(f"Total Backfills: {total_backfills}\n")
        f.write(f"Contiguous Backfills: {contiguous_backfills}\n")
        f.write(f"Basic Backfills: {basic_backfills}\n")
        f.write(f"\nResource Utilization: {utilization:.2f}%\n")
        f.write("\n=== JOB CHARACTERISTICS ===\n")
        f.write(f"Average Resources per Job: {jobs_df['requested_number_of_resources'].mean():.2f}\n")
        f.write(f"Average Execution Time: {jobs_df['execution_time'].mean():.2f} seconds\n")
        f.write(f"Average Waiting Time: {jobs_df['waiting_time'].mean():.2f} seconds\n")
        f.write(f"Average Stretch: {jobs_df['stretch'].mean():.2f}\n")
        f.write("\nResource Request Distribution:\n")
        for res, count in resource_counts.items():
            f.write(f"  {res} resources: {count} jobs ({count/total_jobs*100:.1f}%)\n")
    
    print(f"\nAnalysis complete. Results saved to {output_dir}")

def main():
    # Check if algorithm name is provided
    if len(sys.argv) < 2:
        print("Usage: python analyze_scheduler_performance.py <algorithm_name> [output_dir]")
        print("Example: python analyze_scheduler_performance.py basic_backfill [res/basic_1000_5]")
        sys.exit(1)
    
    # Get the algorithm name and optional output directory
    algorithm_name = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else None
    
    analyze_scheduler_performance(algorithm_name, output_dir)

if __name__ == "__main__":
    main() 