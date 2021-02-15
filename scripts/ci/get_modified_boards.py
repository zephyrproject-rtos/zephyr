#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0

# A script to generate a list of boards that have changed or added and create an
# arguments file for twister to allow running more tests for those boards.

import re, os
import sh
import logging
import argparse
import glob

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
                description="Generate a twister argument for for boards "
                            " that have changed")
    parser.add_argument('-c', '--commits', default=None,
            help="Commit range in the form: a..b")
    return parser.parse_args()

def main():
    boards = set()
    all_boards = set()

    args = parse_args()
    if not args.commits:
        exit(1)

    # pylint does not like the 'sh' library
    # pylint: disable=too-many-function-args,unexpected-keyword-arg
    commit = sh.git("diff", "--name-only", args.commits, **sh_special_args)
    files = commit.split("\n")

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
        print("-p\n%s" %("\n-p\n".join(all_boards)))



if __name__ == "__main__":
    main()
