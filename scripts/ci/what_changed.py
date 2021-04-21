#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2020 Intel Corporation
# Check if full twister is needed.

import os
import sh
import argparse
import fnmatch


if "ZEPHYR_BASE" not in os.environ:
    exit("$ZEPHYR_BASE environment variable undefined.")

repository_path = os.environ['ZEPHYR_BASE']
sh_special_args = {
    '_tty_out': False,
    '_cwd': repository_path
}

def parse_args():
    parser = argparse.ArgumentParser(
                description="Check if change requires full twister")
    parser.add_argument('-c', '--commits', default=None,
            help="Commit range in the form: a..b")
    return parser.parse_args()

def main():
    args = parse_args()
    if not args.commits:
        exit(1)

    # pylint does not like the 'sh' library
    # pylint: disable=too-many-function-args,unexpected-keyword-arg
    commit = sh.git("diff", "--name-only", args.commits, **sh_special_args)
    files = set()
    files.update(commit.split("\n"))

    with open("scripts/ci/twister_ignore.txt", "r") as sc_ignore:
        ignores = sc_ignore.read().splitlines()
        ignores = filter(lambda x: not x.startswith("#"), ignores)

    found = set()
    files = list(filter(lambda x: x, files))

    for pattern in ignores:
        if pattern:
            found.update(fnmatch.filter(files, pattern))

    if sorted(files) != sorted(found):
        print("full")


if __name__ == "__main__":
    main()
