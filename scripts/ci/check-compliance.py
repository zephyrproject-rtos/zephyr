#!/usr/bin/env python
import sys
import subprocess
import re
import os
import xml.etree.ElementTree as ET
from email.utils import parseaddr
import sh
import logging
import argparse
import check_identity

if "ZEPHYR_BASE" not in os.environ:
    logging.error("$ZEPHYR_BASE environment variable undefined.\n")
    exit(1)

logger = None
DOCS_WARNING_FILE = "doc.warnings"

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
                description="Check for coding style and documentation warnings.")
    parser.add_argument('-c', '--commits', default=None,
            help="Commit range in the form: a..b")
    return parser.parse_args()

def get_shas(refspec):

    sha_list = sh.git("rev-list",
        '--max-count={0}'.format(-1 if "." in refspec else 1),
        refspec, **sh_special_args).split()

    return sha_list



def run_gitlint(tc, commit_range):
    proc = subprocess.Popen('gitlint --commits %s' %(commit_range),
            cwd=repository_path, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    msg = ""
    if proc.wait() != 0:
        msg = proc.stdout.read()

    if msg != "":
        failure = ET.SubElement(tc, 'failure', type="failure", message="commit message error on range: %s" %commit_range)
        failure.text = (str(msg))
        return 1

    return 0


def run_checkpatch(tc, commit_range):
    output = None
    out = ""

    diff = subprocess.Popen(('git', 'diff', '%s' %(commit_range)), stdout=subprocess.PIPE)
    try:
        output = subprocess.check_output(('%s/scripts/checkpatch.pl' %repository_path,
            '--mailback', '--no-tree', '-'), stdin=diff.stdout,
            stderr=subprocess.STDOUT, shell=True)

    except subprocess.CalledProcessError as ex:
        m = re.search("([1-9][0-9]*) errors,", str(ex.output))
        if m:
            failure = ET.SubElement(tc, 'failure', type="failure", message="checkpatch issues")
            failure.text = (str(ex.output))
            return 1

    return 0


def check_doc(tc, range):

    if os.path.exists(DOCS_WARNING_FILE) and os.path.getsize(DOCS_WARNING_FILE) > 0:
        with open(DOCS_WARNING_FILE, "r") as f:
            log = f.read()
            failure = ET.SubElement(tc, 'failure', type="failure",
                        message="documentation issues")
            failure.text = (str(log))
        return 1

    return 0



tests = {
        "gitlint": {
            "call": run_gitlint,
            "name": "Commit message style",
            },
        "checkpatch": {
            "call": run_checkpatch,
            "name": "Code style check using checkpatch",
            },
        "documentation": {
            "call": check_doc,
            "name": "New warnings and errors when building documentation",
            }
        }

def run_tests(range):
    run = "Commit Message / Documentation / Coding Style"
    eleTestsuite = None
    fails = 0
    passes = 0
    errors = 0
    total = 0
    filename = "compliance.xml"

    eleTestsuites = ET.Element('testsuites')
    eleTestsuite = ET.SubElement(eleTestsuites, 'testsuite', name=run, time="0",
            tests="%d" %(errors + passes + fails),  failures="%d" %fails,  errors="%d" %errors, skip="0")

    for test in tests.keys():

        total += 1
        eleTestcase = ET.SubElement(eleTestsuite, 'testcase', classname="%s"
                %(test), name="%s" %(tests[test]['name']), time="0")

        fails += tests[test]['call'](eleTestcase, range)

    eleTestsuite.set("tests", "%s" %total)
    eleTestsuite.set("failures",  "%s" %fails)

    result = ET.tostring(eleTestsuites)
    f = open(filename, 'wb')
    f.write(result)
    f.close()
    return fails


def main():
    args = parse_args()
    if not args.commits:
        exit(1)
    fails = run_tests(args.commits)
    print(fails)
    sys.exit(fails)

if __name__ == "__main__":
    #init_logs()
    main()

