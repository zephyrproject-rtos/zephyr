#!/usr/bin/env python3
#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""Compress a binary file using LZ4."""

import argparse

import lz4.block


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("-i", "--input", required=True, help="Input binary file")
    parser.add_argument("-o", "--output", required=True, help="Output LZ4 compressed file")
    return parser.parse_args()


def main():
    args = parse_args()

    with open(args.input, "rb") as fin:
        data = fin.read()

    with open(args.output, "wb") as fout:
        fout.write(lz4.block.compress(data, store_size=False))


if __name__ == "__main__":
    main()
