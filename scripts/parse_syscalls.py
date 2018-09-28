#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import sys
import re
import argparse
import os
import json

api_regex = re.compile(r'''
__syscall\s+                    # __syscall attribute, must be first
([^(]+)                         # type and name of system call (split later)
[(]                             # Function opening parenthesis
([^)]*)                         # Arg list (split later)
[)]                             # Closing parenthesis
''', re.MULTILINE | re.VERBOSE)


def analyze_headers(multiple_directories):
    ret = []

    for base_path in multiple_directories:
        for root, dirs, files in os.walk(base_path):
            for fn in files:

                # toolchain/common.h has the definition of __syscall which we
                # don't want to trip over
                path = os.path.join(root, fn)
                if not fn.endswith(".h") or path.endswith(os.path.join(os.sep, 'toolchain', 'common.h')):
                    continue

                with open(path, "r", encoding="utf-8") as fp:
                    try:
                        result = [(mo.groups(), fn)
                                  for mo in api_regex.finditer(fp.read())]
                    except Exception:
                        sys.stderr.write("While parsing %s\n" % fn)
                        raise

                    ret.extend(result)

    return ret


def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-i", "--include", required=True, action='append',
                        help="Base include directory")
    parser.add_argument(
        "-j", "--json-file", required=True,
        help="Write system call prototype information as json to file")
    args = parser.parse_args()


def main():
    parse_args()

    syscalls = analyze_headers(args.include)

    syscalls_in_json = json.dumps(
        syscalls,
        indent=4,
        sort_keys=True
    )

    # Check if the file already exists, and if there are no changes,
    # don't touch it since that will force an incremental rebuild
    path = args.json_file
    new = syscalls_in_json
    if os.path.exists(path):
        with open(path, 'r') as fp:
            old = fp.read()

        if new != old:
            with open(path, 'w') as fp:
                fp.write(new)
    else:
        with open(path, 'w') as fp:
            fp.write(new)


if __name__ == "__main__":
    main()
