# Copyright (c) 2026 Analog Devices
# SPDX-License-Identifier: Apache-2.0

import argparse
import collections
import os
import pickle
import sys

from common import *  # noqa: F403
from common import ClockDriver
from nxp_syscon import *  # noqa: F403

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', 'python-devicetree', 'src'))


def parse_args():
    parser = argparse.ArgumentParser(
        description="Display clock tree with computed frequencies", allow_abbrev=False
    )
    parser.add_argument("edt_file", help="Path to the pickled EDT file")
    return parser.parse_args()


def get_specifiers(producer_nodes, node):
    entry = producer_nodes[node]
    if 'specifiers' in entry:
        return entry['specifiers']
    return entry['node'].default_specifiers()


def format_freq(hz):
    if hz is None or hz < 0:
        return "? Hz"
    if hz == 0:
        return "0 Hz"
    if hz >= 1000000 and hz % 1000000 == 0:
        return f"{hz // 1000000} MHz"
    if hz >= 1000 and hz % 1000 == 0:
        return f"{hz // 1000} kHz"
    return f"{hz} Hz"


def compute_node_rate(producer_nodes, node, rates):
    # Use cached rate if available
    if node in rates:
        return rates[node]

    if node not in producer_nodes:
        rates[node] = -1
        return -1

    entry = producer_nodes[node]
    driver = entry['node']
    specs = get_specifiers(producer_nodes, node)
    parents = driver.get_parents()

    # Walk up the tree, computing rate recursively
    if parents is None:
        rate = driver.root_recalc(specs)
        if rate is None:
            rate = -1
    elif len(parents) > 1:
        idx = driver.mux_recalc(specs)
        if idx is not None and 0 <= idx < len(parents):
            rate = compute_node_rate(producer_nodes, parents[idx], rates)
        else:
            rate = -1
    else:
        parent_rate = compute_node_rate(producer_nodes, parents[0], rates)
        if parent_rate is not None and parent_rate >= 0:
            result = driver.configure_recalc(specs, parent_rate)
            rate = result if result is not None else -1
        else:
            rate = -1

    rates[node] = rate
    return rate


def node_name(node):
    return node.labels[0] if len(node.labels) > 0 else node.name


def print_node(node, producer_nodes, children, output_children, rates, prefix, is_last, is_root):
    entry = producer_nodes[node]
    driver = entry['node']
    parents = driver.get_parents()
    rate = rates.get(node, -1)

    label = node_name(node)
    if parents is not None and len(parents) > 1:
        specs = get_specifiers(producer_nodes, node)
        idx = driver.mux_recalc(specs)
        if idx is not None and 0 <= idx < len(parents):
            label += f" [mux: {node_name(parents[idx])}]"

    if is_root:
        print(f"{label} ({format_freq(rate)})")
        child_prefix = ""
    else:
        connector = "└── " if is_last else "├── "
        print(f"{prefix}{connector}{label} ({format_freq(rate)})")
        child_prefix = prefix + ("    " if is_last else "│   ")

    kids = sorted(children.get(node, []), key=lambda n: n.name)
    out_kids = sorted(output_children.get(node, []), key=lambda n: n.name)
    total = len(kids) + len(out_kids)

    for i, child in enumerate(kids):
        print_node(
            child,
            producer_nodes,
            children,
            output_children,
            rates,
            child_prefix,
            i == len(kids) - 1 and not out_kids,
            False,
        )

    for i, out_node in enumerate(out_kids):
        is_child_last = len(kids) + i == total - 1
        connector = "└── " if is_child_last else "├── "
        out_rate = rates.get(node, -1)
        print(f"{child_prefix}{connector}{node_name(out_node)} [output] ({format_freq(out_rate)})")


def main():
    args = parse_args()

    compat_map = {}
    producer_nodes = {}
    clock_requests = collections.defaultdict(lambda: collections.defaultdict(int))

    for subclass in ClockDriver.__subclasses__():
        compat_map[subclass.get_dt_compat()] = subclass

    with open(args.edt_file, "rb") as f:
        edt = pickle.load(f)

    # Find all producer nodes, and instantiate objects for them
    for node in edt.nodes:
        for compat in node.compats:
            if compat in compat_map:
                driver_class = compat_map[compat]
                producer_nodes[node] = {'node': driver_class(node)}

    # Calculate votes for each clock state based on clock-request-0 properties
    for node in edt.nodes:
        if "clock-request-0" in node.props and node.status == "okay":
            request = node.props['clock-request-0'].val
            for state in request:
                output = state.parent
                clock_requests[output][state] += 1

    # Find the winning state for each clock output
    for _, states in clock_requests.items():
        best_state = None
        for state, count in states.items():
            if (
                best_state is None
                or count > states[best_state]
                or (
                    count == states[best_state]
                    and state.props['rank'].val < best_state.props['rank'].val
                )
            ):
                best_state = state
        # Record the clock settings for the winning state
        settings = best_state.props['clocks'].val
        for clock_setting in settings:
            if clock_setting.controller not in producer_nodes:
                print(f"Producer node {clock_setting.controller} not found in producer_nodes")
                return os.EX_DATAERR
            producer_nodes[clock_setting.controller]['specifiers'] = clock_setting.data

    children = collections.defaultdict(list)
    for node, entry in producer_nodes.items():
        driver = entry['node']
        parents = driver.get_parents()
        if parents is None:
            continue
        # Find the parent of the producer node, record the relationship
        if len(parents) > 1:
            specs = get_specifiers(producer_nodes, node)
            idx = driver.mux_recalc(specs)
            if idx is not None and 0 <= idx < len(parents):
                parent = parents[idx]
                if parent in producer_nodes:
                    children[parent].append(node)
        elif parents[0] in producer_nodes:
            children[parents[0]].append(node)

    # Find all clock-output nodes and record their parent-child relationships
    output_children = collections.defaultdict(list)
    for node in edt.nodes:
        if "clock-output" in node.compats:
            parent = node.parent
            if parent in producer_nodes:
                output_children[parent].append(node)

    # Find root clocks, sorted by name
    roots = sorted(
        [n for n, e in producer_nodes.items() if e['node'].get_parents() is None],
        key=lambda n: n.name,
    )

    # Compute the effective rate for each producer node
    rates = {}
    for node in producer_nodes:
        compute_node_rate(producer_nodes, node, rates)

    for i, root in enumerate(roots):
        if i > 0:
            print()
        # Recursively print the clock tree starting from the root node
        print_node(root, producer_nodes, children, output_children, rates, "", True, True)


if __name__ == "__main__":
    sys.exit(main())
