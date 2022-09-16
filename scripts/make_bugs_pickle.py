#!/usr/bin/env python3

# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

# stdlib
import argparse
import pickle
import sys
from pathlib import Path
from typing import BinaryIO, List

# third party
from github.Issue import Issue

# other zephyr/scripts modules
from github_helpers import get_github_object

# Note that type annotations are not currently statically checked, and
# should only be considered documentation.

def parse_args() -> argparse.Namespace:
    '''Parse command line arguments.'''
    parser = argparse.ArgumentParser(
        description='''
A helper script which loads all open bugs in the
zephyrproject-rtos/zephyr repository using the GitHub API, and writes
them to a new pickle file as a list of github.Issue.Issue objects.

For more information, see:

  - GitHub API: https://docs.github.com/en/rest
  - github.Issue.Issue:
    https://pygithub.readthedocs.io/en/latest/github_objects/Issue.html
  - pickle: https://docs.python.org/3/library/pickle.html
''',
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('out_file', metavar='OUTFILE', type=Path, nargs='?',
                        help='''file to write pickle data to (default:
                        stdout)''')
    return parser.parse_args()

def get_open_bugs() -> List[Issue]:
    zephyr_repo = get_github_object().get_repo('zephyrproject-rtos/zephyr')
    return list(zephyr_repo.get_issues(state='open', labels=['bug']))

def open_out_file(args: argparse.Namespace) -> BinaryIO:
    if args.out_file is None:
        return open(sys.stdout.fileno(), 'wb', closefd=False)

    return open(args.out_file, 'wb')

def main() -> None:
    args = parse_args()
    open_bugs = get_open_bugs()

    with open_out_file(args) as out_file:
        pickle.dump(open_bugs, out_file)

if __name__ == '__main__':
    main()
