#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
# Copyright (c) 2025 Siemens AG
#
# SPDX-License-Identifier: Apache-2.0


"""Convert a file to a list of hex characters

The list of hex characters can then be included to a source file. Optionally,
the output can be compressed.

"""

import argparse
import codecs
import gzip
import io


def parse_args():
    global args

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)

    parser.add_argument("-f", "--file", required=True, help="Input file")
    parser.add_argument("-o", "--offset", type=lambda x: int(x, 0), default=0,
                        help="Byte offset in the input file")
    parser.add_argument("-l", "--length", type=lambda x: int(x, 0), default=-1,
                        help="""Length in bytes to read from the input file.
                        Defaults to reading till the end of the input file.""")
    parser.add_argument("-m", "--format", default="list",
                        help="Output format: 'list' (default) or 'literal' (string literal)")
    parser.add_argument("-g", "--gzip", action="store_true",
                        help="Compress the file using gzip before output")
    parser.add_argument("-t", "--gzip-mtime", type=int, default=0,
                        nargs='?', const=None,
                        help="""mtime seconds in the gzip header.
                        Defaults to zero to keep builds deterministic. For
                        current date and time (= "now") use this option
                        without any value.""")
    args = parser.parse_args()


def get_nice_string(list_or_iterator):
    # Convert into comma separated list form.
    s = ", ".join("0x" + str(x) for x in list_or_iterator)

    # Format the list to eight values per line.
    return "\n".join(s[i:i+47] for i in range(0, len(s), 48))


def make_hex(chunk):
    hexdata = codecs.encode(chunk, 'hex').decode("utf-8")
    hexlist = map(''.join, zip(*[iter(hexdata)] * 2))
    print(get_nice_string(hexlist) + ',')


def make_string_literal(chunk):
    hexdata = codecs.encode(chunk, 'hex').decode("utf-8")
    hexlist = map(''.join, zip(*[iter(hexdata)] * 2))
    print(''.join("\\x" + str(x) for x in hexlist), end='')


def main():
    parse_args()

    if args.gzip:
        with io.BytesIO() as content:
            with open(args.file, 'rb') as fg:
                fg.seek(args.offset)
                with gzip.GzipFile(fileobj=content, mode='w',
                                   mtime=args.gzip_mtime,
                                   compresslevel=9) as gz_obj:
                    gz_obj.write(fg.read(args.length))

            content.seek(0)
            for chunk in iter(lambda: content.read(8), b''):
                make_hex(chunk)
    else:
        with open(args.file, "rb") as fp:
            fp.seek(args.offset)

            if args.format == "literal":
                if args.length < 0:
                    print('"', end='')
                    for chunk in iter(lambda: fp.read(1024), b''):
                        make_string_literal(chunk)
                    print('"', end='')
                else:
                    print('"', end='')
                    remainder = args.length
                    for chunk in iter(lambda: fp.read(min(1024, remainder)), b''):
                        make_string_literal(chunk)
                        remainder = remainder - len(chunk)
                    print('"', end='')

            else:
                if args.length < 0:
                    for chunk in iter(lambda: fp.read(1024), b''):
                        make_hex(chunk)
                else:
                    remainder = args.length
                    for chunk in iter(lambda: fp.read(min(1024, remainder)), b''):
                        make_hex(chunk)
                        remainder = remainder - len(chunk)


if __name__ == "__main__":
    main()
