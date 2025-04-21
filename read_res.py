import pandas as pd

# Load the CSV file
df = pd.read_csv("./out/jobs.csv")

recomp_waiting = {
    "Original": df["waiting_time"].values,
    "Recomp": df["finish_time"].values - df["starting_time"].values,
}

# Print the results
print("Waiting times for jobs")
for stat, value in recomp_waiting.items():
    print(f"{stat}: {value}")


print("\nWaiting times Stats")
waiting_time_stats = {
    "Minimum": df["waiting_time"].min(),
    "Median": df["waiting_time"].median(),
    "Mean": df["waiting_time"].mean(),
    "Maximum": df["waiting_time"].max(),
}
for stat, value in waiting_time_stats.items():
    print(f"{stat}: {value}")