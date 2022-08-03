#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2022 Martin Schröder

import argparse
import clang.cindex
from clang.cindex import CursorKind

# These are blacklisted because they are used early during init
# meaning that nothing is initialized yet - so we don't wrap them
blacklist = [
    "__errno_location",
    "k_work_queue_start",
    "z_init_static_threads",
    "k_sched_lock",
    "k_sched_unlock",
]


def dump_functions(in_file, out_file):
    with open(out_file, "w") as f_out:
        index = clang.cindex.Index.create()
        tu = index.parse(in_file)
        for c in tu.cursor.walk_preorder():
            if c.kind == CursorKind.FUNCTION_DECL:
                if c.spelling not in blacklist:
                    f_out.write(c.spelling + "\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-i", "--input", type=str, help="Input header to parse", required=True
    )
    parser.add_argument(
        "-o", "--output", type=str, help="Output file for function list", required=True
    )
    args = parser.parse_args()

    dump_functions(args.input, args.output)
