#!/usr/bin/env python3
#
# Copyright (c) 2023 Bjarki Arge Andreasen
#
# SPDX-License-Identifier: Apache-2.0
"""
Script to generate iterable sections from JSON encoded dictionary containing lists of items.
"""

import argparse
import json


def get_tagged_items(filepath: str, tag: str) -> list:
    with open(filepath) as fp:
        return json.load(fp)[tag]


def gen_ld(filepath: str, items: list):
    with open(filepath, "w") as fp:
        for item in items:
            fp.write(f"ITERABLE_SECTION_ROM({item}, Z_LINK_ITERABLE_SUBALIGN)\n")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    parser.add_argument("-i", "--input", required=True, help="Path to input list of tags")
    parser.add_argument("-t", "--tag", required=True, help="Tag to generate iterable sections for")
    parser.add_argument("-l", "--ld-output", required=True, help="Path to output linker file")

    return parser.parse_args()


def main():
    args = parse_args()

    items = get_tagged_items(args.input, args.tag)

    gen_ld(args.ld_output, items)


if __name__ == "__main__":
    main()
