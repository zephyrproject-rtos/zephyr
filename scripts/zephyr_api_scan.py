#!/usr/bin/env python3
#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import argparse
from pathlib import Path
from textwrap import dedent

from zephyr_repo_tools.api_scan import (
    load_api_from_json,
    save_api_to_json,
    scan_api_structs,
)


def print_subsystem_structs(entries, output_file=None):
    """
    Print or write the collected subsystem struct information

    Args:
        entries (list): List of struct entries to print
        output_file (str, optional): File to write results to. If None, prints to stdout.
    """
    # Format output text
    if entries:
        results = []
        for entry in entries:
            mandatory_api_str = "\n".join([f"  - {name}" for name in entry['mandatory_apis']])
            optional_api_str = "\n".join(
                [f"  - {name} [Condition: {cond}]" for name, cond in entry['optional_apis']]
            )
            runtime_optional_api_str = (
                "\n".join([f"  - {name}" for name in entry['runtime_optional_apis']])
                if entry['runtime_optional_apis']
                else "  None"
            )

            wrapper_api_str = "\n".join(
                f"  - {wapi}: {', '.join(entry['wrapper_apis'][wapi])}"
                for wapi in entry['wrapper_apis']
            )

            if not entry['optional_configs']:
                entry['optional_configs'] = 'none'

            mandatory_str = mandatory_api_str if entry['mandatory_apis'] else '  None'
            optional_str = optional_api_str if entry['optional_apis'] else '  None'
            runtime_str = runtime_optional_api_str if entry['runtime_optional_apis'] else '  None'
            wrapper_str = wrapper_api_str if entry['wrapper_apis'] else '  None'

            result = (
                f"File: {entry['file']} (Line {entry['line']})\n"
                f"Type: {entry['type']}\n\n"
                f"Structure:\n{entry['structure']}\n\n"
                f"API Categorization:\n"
                f"Mandatory APIs:\n{mandatory_str}\n\n"
                f"Optional APIs:\n{optional_str}\n\n"
                f"Runtime Optional APIs:\n{runtime_str}\n\n"
                f"Wrapper APIs:\n{wrapper_str}\n\n"
                f"optional_configs:\n{entry['optional_configs']}\n\n"
                f"{'-' * 80}\n"
            )
            results.append(result)

        output_text = f"Found {len(entries)} __subsystem structs\n\n" + "".join(results)
    else:
        output_text = "No __subsystem structs found."

    # Output results
    if output_file:
        with output_file.open('w') as f:
            f.write(output_text)
        print(f"Found {len(entries)} matches. Results written to {output_file}")
    else:
        print(output_text)


def build_parser() -> argparse.ArgumentParser:
    class _Fmt(argparse.ArgumentDefaultsHelpFormatter, argparse.RawTextHelpFormatter):
        pass

    p = argparse.ArgumentParser(
        prog="zephyr_api_scan",
        description="Find and analyze Zephyr __subsystem API structs.",
        allow_abbrev=False,
        epilog=dedent("""\
            Examples:
              # Scan a tree and print everything
              zephyr_api_scan zephyr/
              zephyr_api_scan zephyr/include

              # Save results to JSON
              zephyr_api_scan zephyr/ -j api.json

              # Load from JSON and print only a given struct type
              zephyr_api_scan --load api.json --type gpio_driver_api

              # Scan and write pretty output to a file
              zephyr_api_scan zephyr/ -o report.txt
        """),
        formatter_class=_Fmt,
    )
    p.add_argument("directory", nargs="?", type=Path, help="Directory to search")
    p.add_argument("-o", "--output", type=Path, help="Write formatted report to file")
    p.add_argument("-j", "--json", type=Path, help="Save collection to JSON")
    p.add_argument("-l", "--load", type=Path, help="Load collection from JSON")
    p.add_argument("-t", "--type", help="Limit output to a specific struct type")

    return p


def main():
    parser = build_parser()
    args = parser.parse_args()

    if not args.load and not args.directory:
        parser.error("You must specify either a directory to scan or --load to load from JSON.")

    # Either load the collection or create it
    if args.load:
        collection = load_api_from_json(args.load)
        print(f"Loaded collection from {args.load}")
    else:
        collection = scan_api_structs(args.directory)

    # Save to JSON if requested
    if args.json:
        save_api_to_json(collection, args.json)
        print(f"Saved collection to {args.json}")

    # Print the results
    if args.type:
        entries = collection.get(args.type, [])
    else:
        entries = [entry for entries_list in collection.values() for entry in entries_list]

    print_subsystem_structs(entries, args.output)


if __name__ == "__main__":
    main()
