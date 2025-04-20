#!/usr/bin/env python3
import os
import sys

def generate_machines(num_machines, output_dir="assets/generated/machines"):
    """Generate a machine XML file with the specified number of machines."""
    # Create the directory if it doesn't exist
    os.makedirs(output_dir, exist_ok=True)
    
    # Generate the XML content
    xml_content = f"""<?xml version='1.0'?>
<!DOCTYPE platform SYSTEM "https://simgrid.org/simgrid.dtd">
<platform version="4.1">

<zone id="machines_{num_machines}" routing="Full">
"""
    
    # Add machine nodes
    for i in range(num_machines):
        xml_content += f'    <host id="node_{i}" speed="10Gf"/>\n'
    
    # Add master host
    xml_content += '    <host id="master_host" speed="100Mf"/>\n'
    
    # Close the XML
    xml_content += """</zone>
</platform>"""
    
    # Write to file
    output_file = os.path.join(output_dir, f'machines_{num_machines}.xml')
    with open(output_file, 'w') as f:
        f.write(xml_content)
    
    print(f"Generated machine file: {output_file}")
    return output_file

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python generate_machines.py <num_machines> [output_dir]")
        print("Example: python generate_machines.py 10 assets/generated/machines")
        sys.exit(1)
    
    num_machines = int(sys.argv[1])
    output_dir = sys.argv[2] if len(sys.argv) > 2 else "assets/generated/machines"
    
    generate_machines(num_machines, output_dir) 