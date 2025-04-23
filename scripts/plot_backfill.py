import matplotlib.pyplot as plt
import numpy as np
import re
from collections import defaultdict

def parse_backfill_data(filename):
    """Parse backfill data from file into contiguous job counts."""
    with open(filename) as f:
        lines = f.readlines()
    
    data = []
    for line in lines:
        match = re.search(r'\((\d+), (\d+), (\d+)\)', line)
        if match:
            total = int(match.group(1))
            contiguous = int(match.group(2))
            if total > 0:
                data.append(contiguous)
    
    return np.array(data)

def kernel_density_estimate(data, x_grid, bandwidth=10.0):
    """
    Simple implementation of Gaussian kernel density estimation.
    
    Parameters:
    -----------
    data : array-like
        The input data points
    x_grid : array-like
        The grid points where density is evaluated
    bandwidth : float
        The bandwidth parameter (controls smoothness)
    
    Returns:
    --------
    density : array-like
        The estimated density at each point in x_grid
    """
    n = len(data)
    density = np.zeros_like(x_grid, dtype=float)
    
    # Apply Gaussian kernel to each data point
    for x in data:
        # Gaussian kernel: K(u) = (1/√(2π)) * exp(-u²/2)
        # where u = (x - x_i) / h
        u = (x_grid - x) / bandwidth
        kernel_values = np.exp(-0.5 * u * u) / (np.sqrt(2 * np.pi) * bandwidth)
        density += kernel_values
    
    # Normalize by number of data points
    density /= n
    
    return density

def plot_backfill_distributions(files):
    """Generate PDF visualization with different colors for each line."""
    # Create figure with more space
    plt.figure(figsize=(12, 8))
    
    # Define colors for better distinction - using a professional color palette
    styles = {
        'Basic': {'color': '#1f77b4', 'linewidth': 2.5},               # Blue
        'Best_cont': {'color': '#ff7f0e', 'linewidth': 2.5},           # Orange
        'Force_cont': {'color': '#2ca02c', 'linewidth': 2.5}           # Green
    }
    
    # Find global range for x-axis
    all_data = []
    for path in files.values():
        data = parse_backfill_data(path)
        all_data.extend(data)
    
    x_min = max(50, min(all_data)) if all_data else 50
    x_max = min(450, max(all_data) * 1.1) if all_data else 450
    
    # Create grid for density evaluation
    x_grid = np.linspace(x_min, x_max, 500)
    
    # Process each dataset
    for label, path in files.items():
        data = parse_backfill_data(path)
        
        if len(data) == 0:
            print(f"No data found for {label}")
            continue
        
        # Calculate optimal bandwidth using Silverman's rule of thumb
        # h = 0.9 * min(std, IQR/1.34) * n^(-1/5)
        std = np.std(data)
        iqr = np.percentile(data, 75) - np.percentile(data, 25)
        n = len(data)
        h = 0.9 * min(std, iqr/1.34) * (n ** (-1/5))
        
        # Use at least a minimum bandwidth to ensure smoothness
        bandwidth = max(h, 5.0)
        
        # Compute density using our custom KDE function
        density = kernel_density_estimate(data, x_grid, bandwidth)
        
        # Scale density for visibility (similar to the reference plot)
        max_density = np.max(density)
        if max_density > 0:
            # Scale to have peak around 0.08-0.1 for consistency with reference
            density = density * (0.08 / max_density)
        
        # Plot the PDF curve with solid lines but different colors
        plt.plot(x_grid, density, '-', **styles[label], label=label)
    
    # Improve plot appearance
    plt.xlabel('Number of contiguous jobs', fontsize=14)
    plt.ylabel('Density', fontsize=14)
    plt.title('Contiguous jobs distribution by algorithm', fontsize=16)
    plt.grid(True, alpha=0.3, linestyle='--')
    plt.xticks(fontsize=12)
    plt.yticks(fontsize=12)
    
    # Set axes limits with more space
    plt.xlim(x_min - 10, x_max + 20)
    plt.ylim(0, 0.12)  # A bit more space at the top
    
    # Add legend with descriptive labels
    legend_labels = {
        'Basic': 'Basic backfilling',
        'Best_cont': 'Best effort contiguous',
        'Force_cont': 'Forced local'
    }
    
    # Create custom legend
    handles, labels = plt.gca().get_legend_handles_labels()
    plt.legend([handles[labels.index(label)] for label in legend_labels if label in labels],
               [legend_labels[label] for label in legend_labels if label in labels],
               loc='upper right', frameon=True, framealpha=0.9, fontsize=12)
    
    plt.tight_layout(pad=2.0)  # Add more padding around the plot
    return plt

# Dictionary of files to process
files = {
    'Basic': 'res/backfill/basic_temp.txt',
    'Best_cont': 'res/backfill/best_cont_temp.txt',
    'Force_cont': 'res/backfill/force_cont_temp.txt'
}

# Create and save the plot
plt = plot_backfill_distributions(files)
plt.savefig('res/plots/colored_contiguous_jobs_distribution.png', dpi=300, bbox_inches='tight')
plt.show()