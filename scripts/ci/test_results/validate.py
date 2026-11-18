#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""
Check results documents (zephyr-results) without running twister.

Errors are printed one per line as "file: path: [rule] message".
Exit status: 0 all valid, 1 at least one invalid, 2 usage error.
"""

import argparse
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
os.environ.setdefault('ZEPHYR_BASE', os.path.realpath(os.path.join(HERE, '..', '..', '..')))
sys.path.insert(0, os.path.join(HERE, '..', '..', 'pylib', 'twister'))

from twisterlib import results  # noqa: E402


def main(argv=None):
    parser = argparse.ArgumentParser(
        allow_abbrev=False,
        description='Check zephyr-results documents against the schema and the document rules.',
        epilog='Exit status: 0 all valid, 1 at least one invalid, 2 usage error.',
    )
    parser.add_argument('files', nargs='+', metavar='FILE', help='results document (JSON)')
    args = parser.parse_args(argv)

    bad = 0
    for name in args.files:
        try:
            with open(name, 'rb') as f:
                raw = f.read()
        except OSError as e:
            parser.error(str(e))
        errors = results.validate(raw)
        for line in errors:
            print(f'{name}: {line}')
        if errors:
            bad += 1
        else:
            print(f'{name}: valid')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
