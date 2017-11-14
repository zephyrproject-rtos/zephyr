#!/usr/bin/env python3

import os
import sys
import argparse

ZEPHYR_BASE = os.environ["ZEPHYR_BASE"]
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/"))

from sanity_chk import scl

def parse_args():
    parser = argparse.ArgumentParser(
                description="Check for coding style and documentation warnings.")
    parser.add_argument(
            '-f',
            '--file',
            default=None,
            help="Board file to be parsed"
            )
    parser.add_argument(
            '-o',
            '--out-file',
            default=None,
            help="name of output file"
            )
    return parser.parse_args()

def main():
    args = parse_args()
    schema = scl.yaml_load(os.path.join(os.environ['ZEPHYR_BASE'],
                     "scripts", "sanity_chk", "sanitycheck-platform-schema.yaml"))
    board = scl.yaml_load_verify(args.file, schema)

    if args.out_file:
        outfile = open(args.out_file, "w")
        output =  "/* This file is auto-generated */"
        output += "\n\n\n"
        output += "#define META_BOARD_NAME \"%s\"\n" %(board['name'])
        output += "#define META_BOARD_IDENTIFIER \"%s\"\n" %(board['identifier'])

        if 'supported' in board:
            output += "\n/* Supported Features */\n"
            for s in board['supported']:
                output += "#define META_BOARD_SUPPORTED_%s 1\n" %(s.replace(":","_").upper())

        outfile.write(output)
        outfile.close()

if __name__ == "__main__":
    main()
