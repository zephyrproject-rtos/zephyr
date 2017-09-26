#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

# This converts a file to a list of hex characters which can then
# be included to a source file.
# Optionally, the output can be compressed if needed.

import argparse
import codecs
import gzip
import io

def parse_args():
    global args

    parser = argparse.ArgumentParser(description = __doc__,
            formatter_class = argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-f", "--file", required=True, help="Input file")
    parser.add_argument("-g", "--gzip", action="store_true",
            help="Compress the file using gzip before output")
    args = parser.parse_args()

def get_nice_string(list_or_iterator):
    return ", ".join( "0x" + str(x) for x in list_or_iterator)

def make_hex(chunk):
    hexdata = codecs.encode(chunk, 'hex').decode("utf-8")
    hexlist = map(''.join, zip(*[iter(hexdata)]*2))
    print(get_nice_string(hexlist) + ',')

def main():
    parse_args()

    if args.gzip:
        with open(args.file, 'rb') as fg:
            content = io.BytesIO(gzip.compress(fg.read(), compresslevel=9))
            for chunk in iter(lambda: content.read(8), b''):
                make_hex(chunk)
    else:
        with open(args.file, "rb") as fp:
            for chunk in iter(lambda: fp.read(8), b''):
                make_hex(chunk)

if __name__ == "__main__":
    main()
