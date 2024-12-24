#!/usr/bin/python3
"""
Print Polyspace results on terminal, for easy review.
Copyright (C) 2020-2024 The MathWorks, Inc.
"""

# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import sys
from collections import Counter


def _parse_findings(filename: str, ignore_metrics=True):
    """Parse CSV file"""
    csv_sep = '\t'  # Polyspace default separator

    def consume_header(oneline):
        parts = oneline.split(csv_sep)
        header.extend([p.strip() for p in parts])

    def consume_data(oneline):
        columns = oneline.split(csv_sep)
        return dict(zip(header, columns, strict=True))

    findings = []
    header = []
    with open(filename, encoding="latin-1") as fp:
        for lno, line in enumerate(fp):
            if lno == 0:
                consume_header(line.rstrip())
            else:
                onefind = consume_data(line.rstrip())
                if onefind and (not ignore_metrics or onefind.get('Family', '') != 'Code Metric'):
                    findings.append(onefind)
    # --
    return findings


def print_sorted(mydict, maxlines=0):
    """Print a dict sorted by value, largest first"""

    for lno, cnt_and_fil in enumerate(
        sorted(((cnt, fil) for fil, cnt in mydict.items()), reverse=True), start=1
    ):
        print(f"   {cnt_and_fil[0]} issues in {cnt_and_fil[1]}")
        if lno >= maxlines and maxlines != 0:
            break


def main(argv):
    # 1. command line arguments as required by your script
    parser = argparse.ArgumentParser(description='Print results to console', allow_abbrev=False)
    parser.add_argument('file', type=str, help='output file from polyspace-results-export')
    parser.add_argument('--details', action='store_true', help='print details')
    args = parser.parse_args()

    # 2. parsing the Polyspace files -> report
    findings = _parse_findings(args.file)
    print("-= Polyspace Static Code Analysis results =-")

    # 3. print details
    if args.details:
        for f in findings:
            print(
                f"{f.get('File', 'unknown file')}:"
                + f"{f.get('Line', '?')}:"
                + f"{f.get('Col', '?')}: "
                + f"{f.get('Family', '')} {f.get('Check', 'Defect')}"
            )

    # 3. summary by category and file
    print("By type:")
    family = Counter([f.get('Family', 'Defect') for f in findings])
    print_sorted(family)
    print("Top 10 files:")
    files = Counter([os.path.basename(f.get('File', 'Unknown')) for f in findings])
    print_sorted(files, 10)
    print(f"SCA found {len(findings)} issues in total")
    return 0


if __name__ == "__main__":
    main(sys.argv[1:])
