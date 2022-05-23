#!/usr/bin/env python3
#
# Copyright (c) 2022 Golioth, Inc.
#
# SPDX-License-Identifier: Apache-2.0

from argparse import ArgumentParser
from math import ceil


CHUNK = "This is a fragment of generated C string. "


parser = ArgumentParser(description="Generate C string of arbitrary size")
parser.add_argument("-s", "--size", help="Size of string (without NULL termination)",
                    required=True, type=int)
parser.add_argument("filepath", help="Output filepath")
args = parser.parse_args()


with open(args.filepath, "w", encoding="UTF-8") as fp:
    fp.write('"')
    chunks = CHUNK * ceil(args.size / len(CHUNK))
    fp.write(chunks[:args.size])
    fp.write('"')
