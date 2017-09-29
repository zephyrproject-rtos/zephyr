#!/usr/bin/env python3

# A script to generate a list of boards that have changed or added and create an
# arguemnts file for sanitycheck to allow running more tests for those boards.

import sys
import re, os
from email.utils import parseaddr
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
    log_lev = os.environ.get('LOG_LEVEL', None)
    level = logging.INFO
    global logger

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
                description="Generate a sanitycheck argument for for boards "
                            " that have changed")
    parser.add_argument('-c', '--commits', default=None,
            help="Commit range in the form: a..b")
    return parser.parse_args()

def main():
    boards = set()

    args = parse_args()
    if not args.commits:
        exit(1)

    commit = sh.git("diff","--name-only", args.commits, **sh_special_args)
    files = commit.split("\n")

    for f in files:
        p = re.match("^boards\/[^/]+\/([^/]+)\/", f)
        if p and p.groups():
            boards.add(p.group(1))

    if boards:
        print("-p\n%s" %("\n-p\n".join(boards)))



if __name__ == "__main__":
    main()

