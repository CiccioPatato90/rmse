import json
import random
import os
import sys

def generate_jobs(num_jobs, max_hosts, filename):
    jobs = []
    profiles = {}
    
    # Generate specified number of jobs
    for i in range(1, num_jobs + 1):
        # Random resource requirement between 1 and max_hosts
        res = random.randint(1, max_hosts)
        # Random walltime between 5 and 30
        walltime = random.randint(5, 30)
        # Random submission time between 0 and 100
        subtime = random.randint(0, 100)
        
        job = {
            "id": f"job{i}",
            "profile": f"delay{i}",
            "res": res,
            "walltime": walltime,
            "subtime": subtime
        }
        jobs.append(job)
        
        # Create corresponding profile
        profiles[f"delay{i}"] = {
            "delay": walltime,
            "type": "delay"
        }
    
    # Create the complete job set
    job_set = {
        "description": f"{num_jobs} jobs with varied resource requirements (max {max_hosts} hosts) and execution times",
        "nb_res": max_hosts * 2,  # Set platform size to 2x max_hosts
        "jobs": jobs,
        "profiles": profiles
    }
    
    # Create directory if it doesn't exist
    os.makedirs("assets/generated", exist_ok=True)
    
    # Write to file
    output_path = f"assets/generated/{filename}"
    with open(output_path, "w") as f:
        json.dump(job_set, f, indent=2)
    
    print(f"Generated {num_jobs} jobs with max {max_hosts} hosts and saved to {output_path}")

if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: python generate_jobs_1000.py <num_jobs> <max_hosts> <filename>")
        print("Example: python generate_jobs_1000.py 1000 6 jobs_1000.json")
        sys.exit(1)
    
    num_jobs = int(sys.argv[1])
    max_hosts = int(sys.argv[2])
    filename = sys.argv[3]
    
    generate_jobs(num_jobs, max_hosts, filename) 