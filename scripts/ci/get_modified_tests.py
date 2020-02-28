#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

# A script to generate a list of tests that have changed or added and create an
# arguments file for sanitycheck to allow running those tests with --all

import os
import sh
import logging
import argparse

if "ZEPHYR_BASE" not in os.environ:
    logging.error("$ZEPHYR_BASE environment variable undefined.\n")
    exit(1)

logger = None

repository_path = os.environ['ZEPHYR_BASE']
sh_special_args = {
    '_tty_out': False,
    '_cwd': repository_path
}

def init_logs():
    global logger
    log_lev = os.environ.get('LOG_LEVEL', None)
    level = logging.INFO
    if log_lev == "DEBUG":
        level = logging.DEBUG
    elif log_lev == "ERROR":
        level = logging.ERROR

    console = logging.StreamHandler()
    format = logging.Formatter('%(levelname)-8s: %(message)s')
    console.setFormatter(format)
    logger = logging.getLogger('')
    logger.addHandler(console)
    logger.setLevel(level)

    logging.debug("Log init completed")

def parse_args():
    parser = argparse.ArgumentParser(
                description="Generate a sanitycheck argument for for tests "
                            " that have changed")
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
    files = commit.split("\n")
    tests = set()
    for f in files:
        if f.endswith(".rst"):
            continue
        d = os.path.dirname(f)
        while d:
            if os.path.exists(os.path.join(d, "testcase.yaml")) or os.path.exists(os.path.join(d, "sample.yaml")):
                tests.add(d)
                break
            else:
                d = os.path.dirname(d)

    if tests:
        print("-T\n%s\n--all" %("\n-T\n".join(tests)))

if __name__ == "__main__":
    main()
