#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

"""
Script to scan Zephyr include directories and emit secure call metadata

Secure calls require boilerplate code on both the Non-Secure and Secure sides
to safely cross the ARMv8-M TrustZone boundary. This script performs a text
scan of directories containing C or header files, building a database of
secure call function prototypes. The information is emitted to a JSON file
for processing by gen_secure_calls.py.

The JSON output schema is identical to that of parse_syscalls.py:
    [ [ [return_type_and_name, arg_list], filename, emit_bool ], ... ]
"""

import argparse
import json
import os
import re
import sys
from pathlib import PurePath

regex_flags = re.MULTILINE | re.VERBOSE

secure_call_regex = re.compile(
    r'''
^[ \t]*(?:__secure_call|__secure_call_always_inline)\s+  # __secure_call at line start only
([^(]+)                                                   # type and name (split later)
[(]                                                       # opening parenthesis
([^)]*)                                                   # arg list (split later)
[)]                                                       # closing parenthesis
''',
    regex_flags,
)


def analyze_headers(include_dir, scan_dir):
    secure_call_ret = []
    secure_call_files = dict()

    multiple_directories = set()
    if include_dir:
        multiple_directories |= set(include_dir)
    if scan_dir:
        multiple_directories |= set(scan_dir)

    # Sort for deterministic output
    multiple_directories = sorted(multiple_directories)

    for base_path in multiple_directories:
        for root, dirs, files in os.walk(base_path, topdown=True):
            dirs.sort()
            files.sort()
            for fn in files:
                # toolchain/common.h has the __secure_call definition itself;
                # secure_call.h and secure_call_handler.h are infrastructure
                # headers that must not be scanned for declarations.
                path = os.path.join(root, fn)
                skip_suffixes = (
                    os.path.join(os.sep, 'toolchain', 'common.h'),
                    os.path.join(os.sep, 'zephyr', 'secure_call.h'),
                    os.path.join(os.sep, 'internal', 'secure_call_handler.h'),
                )
                if not (path.endswith(".h") or path.endswith(".c")) or path.endswith(
                    skip_suffixes
                ):
                    continue

                path = PurePath(os.path.normpath(path)).as_posix()

                if path not in secure_call_files:
                    if include_dir and base_path in include_dir:
                        secure_call_files[path] = {"emit": True}
                    else:
                        secure_call_files[path] = {"emit": False}

    for one_file in secure_call_files:
        with open(one_file, encoding="utf-8") as fp:
            try:
                contents = fp.read()
            except Exception:
                sys.stderr.write(f"Error decoding {one_file}\n")
                raise

        fn = os.path.basename(one_file)

        try:
            to_emit = secure_call_files[one_file]["emit"]
            result = []
            for mo in secure_call_regex.finditer(contents):
                groups = mo.groups()
                groups = [re.sub(r'\s+', ' ', group) for group in groups]
                result.append((groups, fn, to_emit))
        except Exception as e:
            sys.stderr.write(f"While parsing {fn}\n")
            raise e

        secure_call_ret.extend(result)

    return secure_call_ret


def update_file_if_changed(path, new):
    if os.path.exists(path):
        with open(path) as fp:
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
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument(
        "-i", "--include",
        required=False,
        action="append",
        help="Include directories recursively scanned for .h files "
             "containing secure calls that must be present in the final "
             "binary. Can be specified multiple times.",
    )
    parser.add_argument(
        "--scan",
        required=False,
        action="append",
        help="Scan directories recursively for .h files containing "
             "secure calls that need stubs generated. Can be specified "
             "multiple times.",
    )
    parser.add_argument(
        "-j", "--json-file",
        required=True,
        help="Write secure call prototype information as JSON to this file",
    )
    args = parser.parse_args()


def main():
    parse_args()
    secure_calls = analyze_headers(args.include, args.scan)
    out = json.dumps(secure_calls, indent=4, sort_keys=True)
    update_file_if_changed(args.json_file, out)


if __name__ == "__main__":
    main()
