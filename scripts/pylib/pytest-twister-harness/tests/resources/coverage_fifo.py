# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""FIFO peer with a command-controlled coverage dump for adapter unit tests."""

import sys
import time
from pathlib import Path


def main():
    fifo = sys.argv[1]
    deadline = time.monotonic() + 5
    while not Path(fifo + '.out').exists() or not Path(fifo + '.in').exists():
        if time.monotonic() >= deadline:
            raise TimeoutError('FIFO files were not created')
        time.sleep(0.01)

    with open(fifo + '.out', 'wb', buffering=0) as output, open(fifo + '.in', 'rb') as commands:
        for command in commands:
            if command.strip() == b'start':
                output.write(b'GCOV_COVERAGE_DUMP_START suite.test\n')
            elif command.strip() == b'end':
                output.write(b'GCOV_COVERAGE_DUMP_END\n')
            elif command.strip() == b'ready':
                output.write(b'READY\n')
            elif command.strip() == b'delayed-end':
                output.write(b'GCOV_COVERAGE_DUMP_START\n')
                time.sleep(0.3)
                output.write(b'GCOV_COVERAGE_DUMP_END\n')


if __name__ == '__main__':
    main()
