#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Script to scan Zephyr include directories and emit system call and subsystem metadata

System calls require a great deal of boilerplate code in order to implement
completely. This script is the first step in the build system's process of
auto-generating this code by doing a text scan of directories containing
header files, and building up a database of system calls and their
function call prototypes. This information is emitted to a generated
JSON file for further processing.

If the output JSON file already exists, its contents are checked against
what information this script would have outputted; if the result is that the
file would be unchanged, it is not modified to prevent unnecessary
incremental builds.
"""

import sys
import re
import argparse
import os
import json

syscall_regex = re.compile(r'''
__syscall\s+                    # __syscall attribute, must be first
([^(]+)                         # type and name of system call (split later)
[(]                             # Function opening parenthesis
([^)]*)                         # Arg list (split later)
[)]                             # Closing parenthesis
''', re.MULTILINE | re.VERBOSE)

subsys_regex = re.compile(r'''
__subsystem\s+                  # __subsystem attribute, must be first
struct\s+                       # struct keyword is next
([^{]+)                         # name of subsystem
[{]                             # Open curly bracket
''', re.MULTILINE | re.VERBOSE)

def analyze_headers(multiple_directories):
    syscall_ret = []
    subsys_ret = []

    for base_path in multiple_directories:
        for root, dirs, files in os.walk(base_path, topdown=True):
            dirs.sort()
            files.sort()
            for fn in files:

                # toolchain/common.h has the definitions of __syscall and __subsystem which we
                # don't want to trip over
                path = os.path.join(root, fn)
                if not fn.endswith(".h") or path.endswith(os.path.join(os.sep, 'toolchain', 'common.h')):
                    continue

                with open(path, "r", encoding="utf-8") as fp:
                    contents = fp.read()

                try:
                    syscall_result = [(mo.groups(), fn)
                                      for mo in syscall_regex.finditer(contents)]
                    subsys_result = [mo.groups()[0].strip()
                                     for mo in subsys_regex.finditer(contents)]
                except Exception:
                    sys.stderr.write("While parsing %s\n" % fn)
                    raise

                syscall_ret.extend(syscall_result)
                subsys_ret.extend(subsys_result)

    return syscall_ret, subsys_ret


def update_file_if_changed(path, new):
    if os.path.exists(path):
        with open(path, 'r') as fp:
            old = fp.read()

        if new != old:
            with open(path, 'w') as fp:
                fp.write(new)
    else:
        with open(path, 'w') as fp:
            fp.write(new)


def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-i", "--include", required=True, action='append',
                        help='''include directories recursively scanned
                        for .h files.  Can be specified multiple times:
                        -i topdir1 -i topdir2 ...''')
    parser.add_argument(
        "-j", "--json-file", required=True,
        help="Write system call prototype information as json to file")
    parser.add_argument(
        "-s", "--subsystem-file", required=True,
        help="Write subsystem name information as json to file")
    args = parser.parse_args()


def main():
    parse_args()

    syscalls, subsys = analyze_headers(args.include)

    # Only write json files if they don't exist or have changes since
    # they will force and incremental rebuild.

    syscalls_in_json = json.dumps(
        syscalls,
        indent=4,
        sort_keys=True
    )
    update_file_if_changed(args.json_file, syscalls_in_json)

    subsys_in_json = json.dumps(
        subsys,
        indent=4,
        sort_keys=True
    )
    update_file_if_changed(args.subsystem_file, subsys_in_json)


if __name__ == "__main__":
    main()
