#!/usr/bin/python
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""
Simply mock for bash script to use with unit tests.
"""

import sys
import time
from argparse import ArgumentParser

from zen_of_python import zen_of_python


def main() -> int:
    parser = ArgumentParser()
    parser.add_argument('--sleep', action='store', default=0, type=float)
    parser.add_argument('--long-sleep', action='store_true')
    parser.add_argument('--return-code', action='store', default=0, type=int)
    parser.add_argument('--exception', action='store_true')

    args = parser.parse_args()

    if args.exception:
        # simulate crashing application
        raise Exception

    if args.long_sleep:
        # prints data and wait for certain time
        for line in zen_of_python:
            print(line, flush=True)
        time.sleep(args.sleep)
    else:
        # prints lines with delay
        for line in zen_of_python:
            print(line, flush=True)
            time.sleep(args.sleep)

    print('End of script', flush=True)
    print('Returns with code', args.return_code, flush=True)
    time.sleep(1)  # give a moment for external programs to collect all outputs
    return args.return_code


if __name__ == '__main__':
    sys.exit(main())
