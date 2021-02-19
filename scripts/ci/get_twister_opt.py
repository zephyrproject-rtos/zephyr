#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2021 Intel Corporation

# A script to generate twister options based on modified files.

import re, os
import sh
import argparse
import glob

if "ZEPHYR_BASE" not in os.environ:
    exit("$ZEPHYR_BASE environment variable undefined.")

repository_path = os.environ['ZEPHYR_BASE']
sh_special_args = {
    '_tty_out': False,
    '_cwd': repository_path
}

def parse_args():
    parser = argparse.ArgumentParser(
                description="Generate twister argument files based on modified file")
    parser.add_argument('-c', '--commits', default=None,
            help="Commit range in the form: a..b")
    return parser.parse_args()

def find_archs(files):
    # we match both arch/<arch>/* and include/arch/<arch> and skip common.
    # Some architectures like riscv require special handling, i.e. riscv
    # directory covers 2 architectures known to twister: riscv32 and riscv64.
    archs = set()

    for f in files:
        p = re.match(r"^arch\/([^/]+)\/", f)
        if not p:
            p = re.match(r"^include\/arch\/([^/]+)\/", f)
        if p:
            if p.group(1) != 'common':
                if p.group(1) == 'riscv':
                    archs.add('riscv32')
                    archs.add('riscv64')
                else:
                    archs.add(p.group(1))

    if archs:
        with open("modified_archs.args", "w") as fp:
            fp.write("-a\n%s" %("\n-a\n".join(archs)))

def find_boards(files):
    boards = set()
    all_boards = set()

    for f in files:
        if f.endswith(".rst") or f.endswith(".png") or f.endswith(".jpg"):
            continue
        p = re.match(r"^boards\/[^/]+\/([^/]+)\/", f)
        if p and p.groups():
            boards.add(p.group(1))

    for b in boards:
        suboards = glob.glob("boards/*/%s/*.yaml" %(b))
        for subboard in suboards:
            name = os.path.splitext(os.path.basename(subboard))[0]
            if name:
                all_boards.add(name)

    if all_boards:
        with open("modified_boards.args", "w") as fp:
            fp.write("-p\n%s" %("\n-p\n".join(all_boards)))

def find_tests(files):
    tests = set()
    for f in files:
        if f.endswith(".rst"):
            continue
        d = os.path.dirname(f)
        while d:
            if os.path.exists(os.path.join(d, "testcase.yaml")) or \
                os.path.exists(os.path.join(d, "sample.yaml")):
                tests.add(d)
                break
            else:
                d = os.path.dirname(d)

    if tests:
        with open("modified_tests.args", "w") as fp:
            fp.write("-T\n%s\n--all" %("\n-T\n".join(tests)))

if __name__ == "__main__":

    args = parse_args()
    if not args.commits:
        exit(1)

    # pylint does not like the 'sh' library
    # pylint: disable=too-many-function-args,unexpected-keyword-arg
    commit = sh.git("diff", "--name-only", args.commits, **sh_special_args)
    files = commit.split("\n")

    find_boards(files)
    find_archs(files)
    find_tests(files)
