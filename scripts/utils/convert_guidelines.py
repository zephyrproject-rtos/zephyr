#!/usr/bin/env python3

# Copyright (c) 2023 Baumer (www.baumer.com)
# SPDX-License-Identifier: Apache-2.0

"""This script converting the Zephyr coding guideline rst file to a output file,
or print the output to the console. Which than can be used by a tool which
needs to have that information in a specific format (e.g. for cppcheck).
Or simply use the rule list to generate a filter to suppress all other rules
used by default from such a tool.
"""

import sys
import re
import argparse
from pathlib import Path

class RuleFormatter:
    """
    Base class for the different output formats
    """
    def table_start_print(self, outputfile):
        pass
    def severity_print(self, outputfile, guideline_number, severity):
        pass
    def description_print(self, outputfile, guideline_number, description):
        pass
    def closing_print(self, outputfile):
        pass

class CppCheckFormatter(RuleFormatter):
    """
    Formatter class to print the rules in a format which can be used by cppcheck
    """
    def table_start_print(self, outputfile):
        # Start search by cppcheck misra addon
        print('Appendix A Summary of guidelines', file=outputfile)

    def severity_print(self, outputfile, guideline_number, severity):
        print('Rule ' + guideline_number + ' ' + severity, file=outputfile)

    def description_print(self, outputfile, guideline_number, description):
        print(description + '(Misra rule ' + guideline_number + ')', file=outputfile)

    def closing_print(self, outputfile):
        # Make cppcheck happy by starting the appendix
        print('Appendix B', file=outputfile)
        print('', file=outputfile)

def convert_guidelines(args):
    inputfile = args.input
    outputfile = sys.stdout
    formatter = None

    # If the output is not empty, open the given file for writing
    if args.output is not None:
        outputfile = open(args.output, "w")

    try:
        file_stream = open(inputfile, 'rt', errors='ignore')
    except Exception:
        print('Error opening ' + inputfile +'.')
        sys.exit()

    # Set formatter according to the used format
    if args.format == 'cppcheck':
        formatter = CppCheckFormatter()

    # Search for table named Main rules
    pattern_table_start = re.compile(r'.*list-table:: Main rules')
    # Each Rule is a new table column so start with '[tab]* -  Rule'
    # Ignore directives here
    pattern_new_line = re.compile(r'^    \* -  Rule ([0-9]+.[0-9]+).*$')
    # Each table column start with '[tab]-  '
    pattern_new_col = re.compile(r'^      -  (.*)$')

    table_start = False
    guideline_number = ''
    guideline_state  = 0
    guideline_list = []

    for line in file_stream:

        line = line.replace('\r', '').replace('\n', '')

        # Done if we find the Additional rules table start
        if line.find('Additional rules') >= 0:
            break

        if len(line) == 0:
            continue

        if not table_start:
            res = pattern_table_start.match(line)
            if res:
                table_start = True
                formatter.table_start_print(outputfile)
            continue

        res = pattern_new_line.match(line)
        if res:
            guideline_state = "severity"
            guideline_number = res.group(1)
            guideline_list.append(guideline_number)
            continue
        elif guideline_number == '':
            continue

        res = pattern_new_col.match(line)
        if res:
            if guideline_state == "severity":
                # Severity
                formatter.severity_print(outputfile, guideline_number, res.group(1))
                guideline_state = "description"
                continue
            if guideline_state == "description":
                # Description
                formatter.description_print(outputfile, guideline_number, res.group(1))
                guideline_state = "None"
                # We stop here for now, we do not handle the CERT C col
                guideline_number = ''
                continue

    formatter.closing_print(outputfile)

if __name__ == "__main__":
    supported_formats = ['cppcheck']

    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument(
        "-i", "--input", metavar="RST_FILE", type=Path, required=True,
        help="Path to rst input source file, where the guidelines are written down."
    )
    parser.add_argument(
        "-f", "--format", metavar="FORMAT", choices=supported_formats, required=True,
        help="Format to convert guidlines to. Supported formats are: " + str(supported_formats)
    )
    parser.add_argument(
        "-o", "--output", metavar="OUTPUT_FILE", type=Path, required=False,
        help="Path to output file, where the converted guidelines are written to. If outputfile is not specified, print to stdout."
    )
    args = parser.parse_args()

    convert_guidelines(args)
