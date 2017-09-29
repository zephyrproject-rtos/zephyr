#!/usr/bin/env python3

# A script to check email identities in commit message. We verify the author
# has signed off using the same email address
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
                description="Verify that author identity matches Signed-off-by")
    parser.add_argument('-c', '--commits', default=None,
            help="Commit range in the form: a..b")
    return parser.parse_args()

def get_shas(refspec):

    sha_list = sh.git("rev-list",
        '--max-count={0}'.format(-1 if "." in refspec else 1),
        refspec, **sh_special_args).split()

    return sha_list

def verify_signed_off(tc, commit):

    signed = []
    author = ""
    sha = ""
    parsed_addr = None
    for line in commit.split("\n"):
        match = re.search("^commit\s([^\s]*)", line)
        if match:
            sha = match.group(1)
        match = re.search("^Author:\s(.*)", line)
        if match:
            author = match.group(1)
            parsed_addr = parseaddr(author)
        match = re.search("signed-off-by:\s(.*)", line, re.IGNORECASE)
        if match:
            signed.append(match.group(1))

    error1 = "%s: author email (%s) needs to match one of the signed-off-by entries." %(sha, author)
    error2 = "%s: author email (%s) does not follow the syntax: First Last <email>." %(sha, author)
    error = 0
    if tc:
        failure = None
        if author not in signed:
            failure = ET.SubElement(tc, 'failure', type="failure", message="identity error on range: %s" %commit_range)
            failure.text = error1
            error = 1
        if not parsed_addr or len(parsed_addr[0].split(" ")) < 2:
            if not failure:
                failure = ET.SubElement(tc, 'failure', type="failure", message="identity error on range: %s" %commit_range)
                failure.text = error2
            else:
                failure.text = failure.text + "\n" + error2
            error = 1

    else:
        if author not in signed:
            print(error1)
            error = 1

        if not parsed_addr or len(parsed_addr[0].split(" ")) < 2:
            print(error2)
            error = 1

    return error

def main():
    args = parse_args()
    if not args.commits:
        exit(1)
    for f in get_shas(args.commits):
        commit = sh.git("log","--decorate=short", "-n 1", f, **sh_special_args)
        verify_signed_off(None, commit)

if __name__ == "__main__":
    main()

