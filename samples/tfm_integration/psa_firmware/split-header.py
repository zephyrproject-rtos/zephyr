#!/usr/bin/env python3
#
# Copyright (c) 2022, Linaro
#
# SPDX-License-Identifier: Apache-2.0
import argparse

from intelhex import IntelHex

def dump_header(infile, image, header):
    inhex = IntelHex(infile)
    (start, end) = inhex.segments()[0]
    inhex.tobinfile(image, start=start, end=end-1)
    (start, end) = inhex.segments()[-1]
    inhex.tobinfile(header, start=start, end=end-1)

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('input')
    parser.add_argument('image')
    parser.add_argument('header')
    args = parser.parse_args()
    dump_header(args.input, args.image, args.header)
