#!/usr/bin/env python3

# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

# stdlib
import argparse
import pickle
from pathlib import Path
from typing import List

# third party
from github.Issue import Issue

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter, allow_abbrev=False)

    parser.add_argument('pickle_file', metavar='PICKLE-FILE', type=Path,
                        help='pickle file containing list of issues')

    return parser.parse_args()

def issue_has_label(issue: Issue, label: str) -> bool:
    for lbl in issue.labels:
        if lbl.name == label:
            return True
    return False

def is_open_bug(issue: Issue) -> bool:
    return (issue.pull_request is None and
            issue.state == 'open' and
            issue_has_label(issue, 'bug'))

def get_bugs(args: argparse.Namespace) -> List[Issue]:
    '''Get the bugs to use for analysis, given command line arguments.'''
    with open(args.pickle_file, 'rb') as f:
        return [issue for issue in pickle.load(f) if
                is_open_bug(issue)]

def main() -> None:
    args = parse_args()
    bugs = get_bugs(args)
    for bug in sorted(bugs, key=lambda bug: bug.number):
        title = bug.title.strip()
        print(f'- :github:`{bug.number}` - {title}')

if __name__ == '__main__':
    main()
