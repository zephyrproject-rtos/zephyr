"""
This module preprocesses the clock management code for an SOC, to generate
functions to apply each setpoint and query each subsystem referenced in the
system devicetree. This is used as an alternative to defining clock
management functions as macros, to simplify debugging for implementers
"""

import argparse
import os
import sys
import pickle
from pathlib import Path
import re

ZEPHYR_BASE = str(Path(__file__).resolve().parents[2])
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "dts",
                                "python-devicetree", "src"))


def parse_args():
    """
    Parses program arguments
    """
    parser = argparse.ArgumentParser(prog="gen_clock_mgmt.py",
                            description="Generate clock management C code",
                            allow_abbrev=False)
    parser.add_argument("edt_pickle", type=str,
                help="edt pickle file describing devicetree")
    parser.add_argument("soc_clocks", type=str,
                help="SOC clock management file to process")
    parser.add_argument("output_file", type=str,
                help="Processed SOC clock management file to create")
    return parser.parse_args()


def find_func_bounds(stream, curr_line, curr_lc):
    """
    Finds the bounds of a function, returning a string with the contents
    of the function
    @param stream file Stream to read the function from
    @param curr_line Portion of the first line to parse, after the function
                     name has been declared
    @param curr_lc Current line count in the file. Used for error reporting.
    @return string containing the function definition. The first line within
            "curr_line" will not be included
    """
    line = curr_line
    line_count = curr_lc
    func_line = curr_line
    setpoint_templ = ""
    brace_level = 0

    # Look for the opening bracket of the function
    while brace_level == 0 and line != "":
        for (idx, char) in enumerate(line):
            if char == '{':
                brace_level = 1
                opening_line = line
                opening_line_num = line_count
                line = line[idx + 1:]
                break
        # Search next line
        if brace_level == 0:
            line = stream.readline()
            line_count += 1
            setpoint_templ += line
    if line == "":
        sys.exit(f"Error: could not find '{{' after function declaration on"
                 f" line {func_line}")
    ## Opening bracket was found. Locate closing bracket.
    while brace_level != 0 and line != "":
        for char in line:
            if char == '{':
                brace_level += 1
            if char == '}':
                brace_level -= 1
            if brace_level == 0:
                break
        setpoint_templ += line
        if brace_level != 0:
            # Search next line
            line = stream.readline()
            line_count += 1
    if line == "":
        sys.exit(f"Error: could not find matching '}}' for '{{' on line "
                f"{opening_line_num}: {opening_line}")
    return (setpoint_templ, line_count)

def gen_node_dictionaries(edt):
    """
    Generate dictionary mapping every node path with setpoints to
    a count of the setpoints for that node, and dictionary mapping
    every node path with clock subsystems to an array of indices with
    subsystems for that node which use clock-output nodes
    @param edt: EDTLib instance describing devicetree
    """
    node_setpoint_counts = {}
    node_clock_indices = {}
    for node in edt.nodes:
        if node.status == "okay":
            clock_state_props = [prop for name, prop in node.props.items()
                                 if re.match("clock-state-[0-9]+", name)]
            if "clocks" in node.props:
                node_clock_indices[f"DT_{node.z_path_id}"] = []
                for (idx, clock_ph) in enumerate(node.props["clocks"].val):
                    if "clock-id" in clock_ph.controller.props:
                        # Add this index to the array
                        node_clock_indices[f"DT_{node.z_path_id}"].append(idx)
            if len(clock_state_props) > 0:
                node_setpoint_counts[f"DT_{node.z_path_id}"] = len(clock_state_props)
    return (node_setpoint_counts, node_clock_indices)



def main():
    """
    Main entry point for script. Generates clock management C code,
    which is used by clock management subsystem to apply clock states
    """
    args = parse_args()
    if os.path.isfile(args.edt_pickle):
        with open(args.edt_pickle, 'rb') as file:
            edt = pickle.load(file)
    else:
        sys.exit(f"Error: could not open edt pickle file {args.edt_pickle}")

    (node_setpoint_counts, node_clock_indices) = gen_node_dictionaries(edt)

    # In order to generate clock management code, we need to define one
    # implementation of the SOC clock get rate function per each clock
    # subsystem used in the devicetree, as well as one implementation of
    # the SOC clock set rate function per each clock setpoint in the devicetree.

    # This process requires us to identify the SOC clock get rate/set rate
    # function within the soc clock management template, and then write
    # instances of these functions per each clock subystem or setpoint.

    output_file = open(args.output_file, "w")

    if not os.path.isfile(args.soc_clocks):
        sys.exit(f"Error: could not open file {args.soc_clocks} for reading")

    line_count = 1
    with open(args.soc_clocks, 'r') as clock_input:
        line = clock_input.readline()
        while line != "":
            setpoint_match = re.match(r'.*Z_CLOCK_MGMT_SETPOINT_TEMPL'
                                    r'\((\w+),\s*(\w+)\)', line)
            subsys_match = re.match(r'.*Z_CLOCK_MGMT_SUBSYS_TEMPL'
                                    r'\((\w+),\s*(\w+),\s*(\w+)\)', line)
            if setpoint_match:
                setpoint_templ = line
                line = line[setpoint_match.span()[1]:]
                (func, line_count) = find_func_bounds(clock_input, line, line_count)
                setpoint_templ += func
                for (node_id, state_cnt) in node_setpoint_counts.items():
                    # Generate a function for each node setpoint
                    for state_idx in range(state_cnt):
                        func = (setpoint_templ
                            .replace(setpoint_match.group(0),
                                f"int Z_CLOCK_MGMT_SETPOINT_FUNC_NAME({node_id}, "
                                f"{state_idx})(void)")
                            .replace(setpoint_match.group(1), node_id)
                            .replace(setpoint_match.group(2), str(state_idx)))
                        output_file.write(func)
            elif subsys_match:
                subsys_templ = line
                line = line[subsys_match.span()[1]:]
                (func, line_count) = find_func_bounds(clock_input, line, line_count)
                subsys_templ += func
                # Generate a function for each node clock subsys
                for (node_id, indices) in node_clock_indices.items():
                    for subsys_idx in indices:
                        func = (subsys_templ
                                .replace(subsys_match.group(0),
                                    f"int Z_CLOCK_MGMT_SUBSYS_FUNC_NAME({node_id}, clocks,"
                                    f"{subsys_idx})(void)")
                                .replace(subsys_match.group(1), node_id)
                                .replace(subsys_match.group(2), 'clocks')
                                .replace(subsys_match.group(3), str(subsys_idx)))
                        output_file.write(func)
            else:
                output_file.write(line)
            line = clock_input.readline()
            line_count += 1

    # Close output file, to clean up
    output_file.close()

if __name__ == "__main__":
    main()
