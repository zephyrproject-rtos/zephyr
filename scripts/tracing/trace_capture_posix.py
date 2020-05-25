#!/usr/bin/env python3

# Copyright (c) 2020 Intel Corporation.
#
# SPDX-License-Identifier: Apache-2.0
"""
Script to capture tracing data with posix backend.
"""

import argparse
import sys
import os

def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-o", "--output", default='channel0_0',
                        required=False, help="capture tracing data with \
                        posix backend.")
    args = parser.parse_args()

def main():
    parse_args()
    output_file = args.output
    os.environ['output_file'] = str(output_file)

    # capture the tracing data with posix backend in 5 sec.
    print("tracing data capturing  ...")
    os.system("$ZEPHYR_BASE/build/zephyr/zephyr.exe \
        -trace-file=$output_file & sleep 5 && \
        ps -ef|grep zephyr.exe|awk '{print $2}'|xargs kill -9 \
        2>/dev/null")

    if os.path.exists(output_file) and os.path.getsize(output_file) > 0:
        raise KeyboardInterrupt
    else:
        print("fail to capture the tracing.")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print('Done, tracing data saved into {}'.format(
            args.output))
        sys.exit(0)
