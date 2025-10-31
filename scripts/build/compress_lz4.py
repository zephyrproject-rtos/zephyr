#!/usr/bin/env python3

# Copyright 2025 CISPA Helmholtz Center for Information Security gGmbH
# SPDX-License-Identifier: Apache-2.0

"""
Compresses a file with lz4 compression.

Reads a file to memory, compresses the contents with lz4
and overwrites the file with the compressed contents.
Uses python lz4 library to alleviate need for lz4 CLI utility.
"""

import argparse

import lz4.frame


def parse_args() -> tuple[str, bool, int]:
    global args

    parser = argparse.ArgumentParser(prog="LZ4 compressor", description=__doc__, allow_abbrev=False)

    parser.add_argument("filename")
    parser.add_argument("-q", "--quiet", action='store_true', help="Suppress statistics")
    parser.add_argument(
        "-c",
        "--compression-level",
        default=1,
        type=int,
        help="Compression level (12=best, 1=fastest)",
    )

    args = parser.parse_args()

    return (args.filename, args.quiet, args.compression_level)


def compress(filename: str, quiet: bool, compression_level: int) -> None:
    with open(filename, "rb") as file:
        in_data = file.read()
    compressed = lz4.frame.compress(
        in_data, compression_level=compression_level, content_checksum=True, store_size=True
    )
    with open(filename, "wb") as file:
        file.write(compressed)
    if not quiet:
        print(
            f"Compressed extension {filename}:{len(compressed)}/{len(in_data)} bytes"
            + f" ({(len(compressed) / len(in_data)) * 100})%"
        )


if __name__ == "__main__":
    filename, quiet, compression_level = parse_args()
    compress(filename, quiet, compression_level)
