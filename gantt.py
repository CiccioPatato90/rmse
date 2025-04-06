from evalys.jobset import JobSet
import matplotlib.pyplot as plt
import sys
import os

# Check if a filename was provided as an argument
if len(sys.argv) < 2:
    print("Usage: python gantt.py <algorithm_name>")
    print("Example: python gantt.py basic_backfill")
    sys.exit(1)

# Get the algorithm name from command line arguments
algorithm_name = sys.argv[1]

# Define input and output paths
#input_csv = f"./out/{algorithm_name}_jobs.csv"
input_csv = "./out/jobs.csv"
output_dir = "./res"
output_file = f"{output_dir}/{algorithm_name}_gantt.png"

# Check if input file exists
if not os.path.exists(input_csv):
    print(f"Error: Input file '{input_csv}' not found.")
    sys.exit(1)

# Create output directory if it doesn't exist
os.makedirs(output_dir, exist_ok=True)

# Load job data and create Gantt chart
print(f"Generating Gantt chart for {algorithm_name}...")
js = JobSet.from_csv(input_csv)
js.plot(with_details=True)

# Save the figure
plt.savefig(output_file)
print(f"Gantt chart saved to {output_file}")