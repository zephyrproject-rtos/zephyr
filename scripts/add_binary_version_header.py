#!/usr/bin/python
# -*- coding: utf-8 -*-

import ctypes
import sys
import argparse
import hashlib
import binary_version_header

def main(argv):
    parser = argparse.ArgumentParser(description="""Overwrite the binary
        version header in the passed binary file.""")
    parser.add_argument('--major', dest='major', type=int,
        help='major version number', required=True)
    parser.add_argument('--minor', dest='minor', type=int,
        help='minor version number', required=True)
    parser.add_argument('--patch', dest='patch', type=int,
        help='patch version number', required=True)
    parser.add_argument('--version_string', dest='version_string',
        help='human friendly version string, free format', required=True)
    parser.add_argument('--header_position_hint', dest='header_position_hint',
        type=str, help='one of start, end, unknown. Default to unknown.',
        default="unknown")
    parser.add_argument('input_file',
        help='the binary file to modify in place')
    args = parser.parse_args()

    arr = bytearray(open(args.input_file, "rb").read())

    if args.header_position_hint == 'start':
        header_pos = 0
    elif args.header_position_hint == 'unknown':
        header_pos = arr.find(binary_version_header.MAGIC.encode())
        if header_pos == -1:
            raise Exception("Cannot find the magic string %s in the passed binary." % binary_version_header.MAGIC)
        else:
            print ("Found header magic at offset %x" % header_pos)
        # check of magic to avoid binary corruption
        magic_count = arr.count(binary_version_header.MAGIC.encode())
        if magic_count > 1:
            raise Exception("Multiple magic string found %d times, risk of corruption => stop." % magic_count)
    else:
        raise Exception("Invalid value for header_position_hint argument.")

    # Retrieve version header structure from binary file
    bh = binary_version_header.ctypes_factory(arr[header_pos:])
    structure_size = ctypes.sizeof(bh)
    assert bh.magic == binary_version_header.MAGIC.encode()

    assert bh.version in binary_version_header.VERSION

    # Compute the hash. The header can be anywhere within the binary so we feed
    # the generator in 2 passes
    m = hashlib.sha1()
    m.update(arr[0:header_pos])
    m.update(arr[header_pos+structure_size:])
    digest = bytearray(m.digest())

    bh.major = args.major
    bh.minor = args.minor
    bh.patch = args.patch

    for i in range(0, ctypes.sizeof(bh.hash)):
        bh.hash[i] = digest[i]
    if len(args.version_string) < ctypes.sizeof(bh.version_string):
        args.version_string += '\0' * (ctypes.sizeof(bh.version_string)-len(args.version_string))
    vs = bytearray(args.version_string.encode())
    for i in range(0, ctypes.sizeof(bh.version_string)):
        bh.version_string[i] = vs[i]
    bh.offset = -header_pos
    bh.size  = len(arr)


     # Over-write the header content
    arr[header_pos:header_pos+structure_size] = bytearray(bh)
    magic = binary_version_header.MAGIC
    header_pos = arr.find(magic.encode())

    out_file = open(args.input_file, "wb")
    out_file.write(arr)

if __name__ == "__main__":
    main(sys.argv)
