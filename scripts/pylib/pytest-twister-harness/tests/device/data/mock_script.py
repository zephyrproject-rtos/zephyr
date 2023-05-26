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

s = """
The Zen of Python, by Tim Peters

Beautiful is better than ugly.
Explicit is better than implicit.
Simple is better than complex.
Complex is better than complicated.
Flat is better than nested.
Sparse is better than dense.
Readability counts.
Special cases aren't special enough to break the rules.
Although practicality beats purity.
Errors should never pass silently.
Unless explicitly silenced.
In the face of ambiguity, refuse the temptation to guess.
There should be one-- and preferably only one --obvious way to do it.
Although that way may not be obvious at first unless you're Dutch.
Now is better than never.
Although never is often better than *right* now.
If the implementation is hard to explain, it's a bad idea.
If the implementation is easy to explain, it may be a good idea.
Namespaces are one honking great idea -- let's do more of those!
"""


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
        for line in s.split('\n'):
            print(line, flush=True)
        time.sleep(args.sleep)
    else:
        # prints lines with delay
        for line in s.split('\n'):
            print(line, flush=True)
            time.sleep(args.sleep)

    print('End of script', flush=True)
    print('Returns with code', args.return_code, flush=True)
    return args.return_code


if __name__ == '__main__':
    sys.exit(main())
