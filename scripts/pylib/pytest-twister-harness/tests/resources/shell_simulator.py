# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""
Simple shell simulator.
"""
import sys

from zen_of_python import zen_of_python

PROMPT = 'uart:~$ '


def main() -> int:
    print('Start shell simulator', flush=True)
    print(PROMPT, end='', flush=True)
    for line in sys.stdin:
        line = line.strip()
        print(line, flush=True)
        if line == 'quit':
            break
        elif line == 'zen':
            for zen_line in zen_of_python:
                print(zen_line, flush=True)

        print(PROMPT, end='', flush=True)


if __name__ == '__main__':
    main()
