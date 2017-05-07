#!/usr/bin/env python
import sys
import subprocess
import re
import os
import xml.etree.ElementTree as ET

DOCS_WARNING_FILE = "doc.warnings"

commit_range = os.environ['COMMIT_RANGE']
cwd = os.environ['ZEPHYR_BASE']

def run_gitlint(tc):
    proc = subprocess.Popen('gitlint --commits %s' %(commit_range),
            cwd=cwd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

    msg = ""
    if proc.wait() != 0:
        msg = proc.stdout.read()

    if msg != "":
        failure = ET.SubElement(tc, 'failure', type="failure", message="commit message error on range: %s" %commit_range)
        failure.text = (str(msg))
        return 1

    return 0


def run_checkpatch(tc):
    output = None
    out = ""

    diff = subprocess.Popen(('git', 'diff', '%s' %(commit_range)), stdout=subprocess.PIPE)
    try:
        output = subprocess.check_output(('%s/scripts/checkpatch.pl' %cwd,
            '--mailback', '--no-tree', '-'), stdin=diff.stdout,
            stderr=subprocess.STDOUT, shell=True)

    except subprocess.CalledProcessError as ex:
        m = re.search("([1-9][0-9]*) errors,", str(ex.output))
        if m:
            failure = ET.SubElement(tc, 'failure', type="failure", message="checkpatch issues")
            failure.text = (str(ex.output))
            return 1

    return 0


def check_doc(tc):

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

def run_tests():
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

        fails += tests[test]['call'](eleTestcase)


    eleTestsuite.set("tests", "%s" %total)
    eleTestsuite.set("failures",  "%s" %fails)

    result = ET.tostring(eleTestsuites)
    f = open(filename, 'wb')
    f.write(result)
    f.close()
    return fails

fails = run_tests()
print(fails)
sys.exit(fails)
