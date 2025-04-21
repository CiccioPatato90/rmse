#!/usr/bin/env python3

""" 
Script to analyze the results of the makespan experiments.
The results are stored in the res/res_makespan directory, for each simulated algorithm.
The result helps us build the visualizations used in the paper to describe makespan distribution.

"""
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd
import re
from scipy.stats import gaussian_kde
import os

# Create output directory if it doesn't exist
os.makedirs("res/plots", exist_ok=True)

def read_makespan_data(filename):
    """Read makespan values from the result file."""
    data = []
    with open(filename, 'r') as f:
        # Skip header lines
        for _ in range(3):
            next(f)
        for line in f:
            try:
                sim_num, makespan = line.strip().split(' ')
                makespan = makespan.strip()
                if makespan != 'FAILED':
                    data.append(float(makespan))
            except (ValueError, IndexError):
                continue
    return np.array(data)


def create_comparison_plot(data1, data2, label1, label2, title):
    """Create a scatter plot comparing two algorithms."""
    plt.figure(figsize=(10, 8))
    
    # Filter out extreme outliers (very small values that are likely errors)
    mask = (data1 > 300) & (data2 > 300)
    data1_filtered = data1[mask]
    data2_filtered = data2[mask]
    
    # Calculate percentage difference from basic
    differences = ((data2_filtered - data1_filtered) / data1_filtered) * 100
    avg_diff = np.mean(differences)
    
    # Calculate the min and max for axis limits
    min_val = min(min(data1_filtered), min(data2_filtered))
    max_val = max(max(data1_filtered), max(data2_filtered))
    
    # Add some padding to the limits
    range_val = max_val - min_val
    min_val -= range_val * 0.05
    max_val += range_val * 0.05
    
    # Create diagonal line for reference
    plt.plot([min_val, max_val], [min_val, max_val], 'k--', alpha=0.5, label='y=x (equal performance)')
    
    # Color points based on their relative performance
    better_mask = data2_filtered < data1_filtered
    worse_mask = data2_filtered > data1_filtered
    equal_mask = np.isclose(data2_filtered, data1_filtered, rtol=0.0001)  # 1% tolerance
    
    # Plot points with different colors based on performance
    plt.scatter(data1_filtered[better_mask], data2_filtered[better_mask], 
                alpha=0.6, s=30, c='green', label=f'{label2} better')
    plt.scatter(data1_filtered[worse_mask], data2_filtered[worse_mask], 
                alpha=0.6, s=30, c='red', label=f'{label2} worse')
    plt.scatter(data1_filtered[equal_mask], data2_filtered[equal_mask], 
                alpha=0.6, s=30, c='blue', label='Equal performance')
    
    # Add labels and title
    plt.xlabel(f'Makespan for {label1} (seconds)')
    plt.ylabel(f'Makespan for {label2} (seconds)')
    plt.title(title)
    
    # Add text box with statistics
    stats_text = (
        f'Performance difference: {abs(avg_diff):.1f}%\n'
        f'({"higher" if avg_diff > 0 else "lower"} than {label1})\n'
        f'Better: {sum(better_mask)} cases\n'
        f'Worse: {sum(worse_mask)} cases\n'
        f'Equal: {sum(equal_mask)} cases'
    )
    plt.text(0.05, 0.95, stats_text,
             transform=plt.gca().transAxes,
             bbox=dict(facecolor='white', alpha=0.8),
             verticalalignment='top')
    
    # Make plot square and set equal scales
    plt.axis('square')
    plt.grid(True, alpha=0.3)
    plt.xlim(min_val, max_val)
    plt.ylim(min_val, max_val)
    
    # Add legend
    plt.legend(loc='lower right')
    
    return plt, avg_diff

def main():
    # Read data from files
    basic_data = read_makespan_data('res/makespan/basic_temp.txt')
    best_cont_data = read_makespan_data('res/makespan/best_cont_temp.txt')
    force_cont_data = read_makespan_data('res/makespan/force_cont_temp.txt')
    
    print("\nAnalysis Results:")
    print("-----------------")
    
    # 1. Basic vs Best-effort contiguous
    plt1, diff1 = create_comparison_plot(
        basic_data, best_cont_data,
        'basic backfilling',
        'best effort contiguous',
        'Makespan distribution for basic backfilling and best effort contiguous'
    )
    plt1.savefig('res/plots/basic_vs_best_cont.png', dpi=300, bbox_inches='tight')
    plt1.close()
    
    print("\nBasic vs Best-effort Contiguous:")
    print(f"Average performance difference: {abs(diff1):.1f}%")
    
    # 2. Basic vs Forced contiguous
    plt2, diff2= create_comparison_plot(
        basic_data, force_cont_data,
        'basic backfilling',
        'forced contiguous',
        'Makespan distribution for basic backfilling and forced contiguous'
    )
    plt2.savefig('res/plots/basic_vs_force_cont.png', dpi=300, bbox_inches='tight')
    plt2.close()
    

if __name__ == "__main__":
    main() 