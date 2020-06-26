#!/usr/bin/env python3
#
# Copyright (c) 2019 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""Logging reader tool"""

import argparse
import os
import sys
from lib.loglist import Loglist

QEMU_ETRACE = "/dev/shm/qemu-bridge-hp-sram-mem"
SOF_ETRACE = "/sys/kernel/debug/sof/etrace"

def parse_args():
    """Parsing command line arguments"""

    parser = argparse.ArgumentParser(description='Logging tool')

    parser.add_argument('-e', '--etrace', choices=['sof', 'qemu'], default="sof",
                        help="Specify the trace target")

    parser.add_argument('-f', '--file', help="Specify the trace file created by"
                        " dump_trace tool")

    args = parser.parse_args()

    return args

def main():
    """Main"""

    args = parse_args()

    if os.geteuid() != 0:
        sys.exit("Please run this program as root / sudo")

    if args.file is not None:
        etrace = args.file
    else:
        if args.etrace == 'sof':
            etrace = SOF_ETRACE
        else:
            etrace = QEMU_ETRACE

    l = Loglist(etrace)
    l.print()

if __name__ == "__main__":

    main()
