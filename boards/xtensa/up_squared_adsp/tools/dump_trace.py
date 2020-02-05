#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import os
import argparse
import logging

from lib.device import Device
from lib.etrace import Etrace

def parse_args():

    arg_parser = argparse.ArgumentParser(description="Dump trace message")

    arg_parser.add_argument("-o", "--output-file", default=None,
                            help="Save to output file")
    arg_parser.add_argument("-d", "--debug", default=False, action='store_true',
                            help="Display debug information")
    arg_parser.add_argument("-x", "--hexdump", default=False, action='store_true',
                            help="Display hexdump")

    args = arg_parser.parse_args()

    return args

def main():
    """ Main Entry Point """

    args = parse_args()

    log_level = logging.INFO
    if args.debug:
        log_level = logging.DEBUG

    logging.basicConfig(level=log_level, format="%(message)s")

    dev = Device()
    dev.open_device()

    etrace = Etrace(dev)
    etrace.print()

    if args.hexdump:
        etrace.hexdump()

    if args.output_file:
        etrace.save(args.output_file)


if __name__ == "__main__":
    try:
        main()
        os._exit(0)
    except KeyboardInterrupt:  # Ctrl-C
        os._exit(14)
    except SystemExit:
        raise
    except BaseException:
        import traceback

        traceback.print_exc()
        os._exit(16)
