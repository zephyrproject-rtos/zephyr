#!/usr/bin/env python3
#
# Copyright (c) 2018 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import sys
import subprocess
import re
import os
from email.utils import parseaddr
import sh
import logging
import argparse
from junitparser import TestCase, TestSuite, JUnitXml, Skipped, Error, Failure, Attr
from github import Github
from shutil import copyfile
import json
import tempfile
from colorama import Fore, Back, Style
import glob
import yaml

logger = None

def info(what):
    sys.stdout.write(what + "\n")
    sys.stdout.flush()

def error(what):
    sys.stderr.write(Fore.RED + what + Style.RESET_ALL + "\n")

sh_special_args = {
    '_tty_out': False,
    '_cwd': os.getcwd()
}


def get_shas(refspec):
    """
    Get SHAs from the Git tree.

    :param refspec:
    :return:
    """
    sha_list = sh.git("rev-list",
                      '--max-count={0}'.format(-1 if "." in refspec else 1),
                      refspec, **sh_special_args).split()
    return sha_list


class MyCase(TestCase):
    """
    Implementation of TestCase specific to our tests.

    """
    classname = Attr()
    doc = Attr()


class ComplianceTest:
    """
    Main Test class

    """

    _name = ""
    _title = ""
    _doc = "https://docs.zephyrproject.org/latest/contribute/"

    def __init__(self, suite, commit_range):
        self.case = None
        self.suite = suite
        self.commit_range = commit_range
        self.repo_path = os.getcwd()
        # get() defaults to None if not present
        self.zephyr_base = os.environ.get('ZEPHYR_BASE')

    def prepare(self):
        """
        Prepare test case
        :return:
        """
        self.case = MyCase(self._name)
        self.case.classname = "Guidelines"
        print("Running {} tests...".format(self._name))

    def run(self):
        """
        Run testcase
        :return:
        """
        pass

    def error(self, msg):
        """
        Signals a problem with running the test, with message 'msg'.

        Raises an exception internally, so you do not need to put a 'return'
        after error().

        Any failures generated prior to the error() are included automatically
        in the message. Usually, any failures would indicate problems with the
        test code.
        """
        if self.case.result:
            msg += "\n\nFailures before error: " + self.case.result._elem.text

        self.case.result = Error(msg, "error")

        raise EndTest

    def skip(self, msg):
        """
        Signals that the test should be skipped, with message 'msg'.

        Raises an exception internally, so you do not need to put a 'return'
        after error().

        Any failures generated prior to the skip() are included automatically
        in the message. Usually, any failures would indicate problems with the
        test code.
        """
        if self.case.result:
            msg += "\n\nFailures before skip: " + self.case.result._elem.text

        self.case.result = Skipped(msg, "skipped")

        raise EndTest

    def add_failure(self, msg):
        """
        Signals that the test failed, with message 'msg'. Can be called many
        times within the same test to report multiple failures.
        """
        if not self.case.result:
            # First reported failure
            self.case.result = Failure(self._name + " issues", "failure")
            self.case.result._elem.text = ""

        # If there are multiple Failures, concatenate their messages
        self.case.result._elem.text += msg + "\n\n"


class EndTest(Exception):
    """
    Raised by ComplianceTest.error()/skip() to end the test.

    Tests can raise EndTest themselves to immediately end the test, e.g. from
    within a nested function call.
    """


class CheckPatch(ComplianceTest):
    """
    Runs checkpatch and reports found issues

    """
    _name = "checkpatch"
    _doc = "https://docs.zephyrproject.org/latest/contribute/#coding-style"

    def run(self):
        self.prepare()
        # Default to Zephyr's checkpatch if ZEPHYR_BASE is set
        checkpatch = os.path.join(self.zephyr_base or self.repo_path, 'scripts',
                                  'checkpatch.pl')
        if not os.path.exists(checkpatch):
            self.skip(checkpatch + " not found")

        diff = subprocess.Popen(('git', 'diff', '%s' % (self.commit_range)),
                                stdout=subprocess.PIPE)
        try:
            subprocess.check_output((checkpatch, '--mailback', '--no-tree', '-'),
                                    stdin=diff.stdout,
                                    stderr=subprocess.STDOUT, shell=True)

        except subprocess.CalledProcessError as ex:
            output = ex.output.decode("utf-8")
            if re.search("[1-9][0-9]* errors,", output):
                self.add_failure(output)


class KconfigCheck(ComplianceTest):
    """
    Checks is we are introducing any new warnings/errors with Kconfig,
    for example using undefiend Kconfig variables.
    """
    _name = "Kconfig"
    _doc = "https://docs.zephyrproject.org/latest/application/kconfig-tips.html"

    def run(self):
        self.prepare()

        kconf = self.parse_kconfig()

        self.check_no_undef_within_kconfig(kconf)
        self.check_top_menu_not_too_long(kconf)

    def get_modules(self, modules_file):
        """
        Get a list of modules and put them in a file that is parsed by
        Kconfig

        This is needed to complete Kconfig sanity tests.

        """
        # Invoke the script directly using the Python executable since this is
        # not a module nor a pip-installed Python utility
        zephyr_module_path = os.path.join(self.zephyr_base, "scripts",
                                          "zephyr_module.py")
        cmd = [sys.executable, zephyr_module_path,
               '--kconfig-out', modules_file]
        try:
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                 stderr=subprocess.PIPE)
            p.communicate()
        except subprocess.CalledProcessError as ex:
            self.skip("Command not found: " + str(e))
        except sh.ErrorReturnCode as e:
            self.skip(e)

    def parse_kconfig(self):
        """
        Returns a kconfiglib.Kconfig object for the Kconfig files. We reuse
        this object for all tests to avoid having to reparse for each test.
        """
        if not self.zephyr_base:
            self.skip("Not a Zephyr tree (ZEPHYR_BASE unset)")

        # Put the Kconfiglib path first to make sure no local Kconfiglib version is
        # used
        kconfig_path = os.path.join(self.zephyr_base, "scripts", "kconfig")
        if not os.path.exists(kconfig_path):
            self.error(kconfig_path + " not found")

        sys.path.insert(0, kconfig_path)
        import kconfiglib

        # Look up Kconfig files relative to ZEPHYR_BASE
        os.environ["srctree"] = self.zephyr_base

        # Parse the entire Kconfig tree, to make sure we see all symbols
        os.environ["SOC_DIR"] = "soc/"
        os.environ["ARCH_DIR"] = "arch/"
        os.environ["BOARD_DIR"] = "boards/*/*"
        os.environ["ARCH"] = "*"
        os.environ["CMAKE_BINARY_DIR"] = tempfile.gettempdir()
        os.environ['GENERATED_DTS_BOARD_CONF'] = "dummy"

        # For multi repo support
        self.get_modules(os.path.join(tempfile.gettempdir(), "Kconfig.modules"))

        # Enable strict Kconfig mode in Kconfiglib, which assumes there's just a
        # single Kconfig tree and warns for all references to undefined symbols
        os.environ["KCONFIG_STRICT"] = "y"

        try:
            return kconfiglib.Kconfig()
        except kconfiglib.KconfigError as e:
            self.add_failure(str(e))
            raise EndTest

    def check_no_undef_within_kconfig(self, kconf):
        """
        Checks that there are no references to undefined Kconfig symbols within
        the Kconfig files
        """
        undef_ref_warnings = [warning for warning in kconf.warnings
                              if "undefined symbol" in warning]

        if undef_ref_warnings:
            self.add_failure("Undefined Kconfig symbols:\n\n"
                             + "\n\n\n".join(undef_ref_warnings))

    def check_top_menu_not_too_long(self, kconf):
        """
        Checks that there aren't too many items in the top-level menu (which
        might be a sign that stuff accidentally got added there)
        """
        max_top_items = 50

        n_top_items = 0
        node = kconf.top_node.list
        while node:
            # Only count items with prompts. Other items will never be
            # shown in the menuconfig (outside show-all mode).
            if node.prompt:
                n_top_items += 1
            node = node.next

        if n_top_items > max_top_items:
            self.add_failure("""
Expected no more than {} potentially visible items (items with prompts) in the
top-level Kconfig menu, found {} items. If you're deliberately adding new
entries, then bump the 'max_top_items' variable in {}.
""".format(max_top_items, n_top_items, __file__))


class Codeowners(ComplianceTest):
    """
    Check if added files have an owner.
    """
    _name = "Codeowners"
    _doc  = "https://help.github.com/articles/about-code-owners/"

    def parse_codeowners(self, git_root, codeowners):
        all_files = []
        with open(codeowners, "r") as codeo:
            for line in codeo.readlines():
                if not line.startswith("#") and line != "\n":
                    match = re.match("([^\s]+)\s+(.*)", line)
                    if match:
                        add_base = False
                        path = match.group(1)
                        if path.startswith("/"):
                            abs_path = git_root + path
                        else:
                            abs_path = "**/{}".format(path)
                            add_base = True

                        if abs_path.endswith("/"):
                            abs_path = abs_path + "**"
                        elif os.path.isdir(abs_path):
                            self.add_failure("Expected / after directory '{}' "
                                             "in CODEOWNERS".format(path))
                            continue
                        g = glob.glob(abs_path, recursive=True)
                        if not g:
                            self.add_failure("Path '{}' not found, in "
                                             "CODEOWNERS".format(path))
                        else:
                            files = []
                            if not add_base:
                                for f in g:
                                    l = f.replace(git_root + "/", "")
                                    files.append(l)
                            else:
                                files = g

                            all_files += files

                        maintainers = match.group(2).split(" ")

        files = []
        for f in all_files:
            if os.path.isfile(f):
                files.append(f)

        return set(files)

    def run(self):
        self.prepare()
        git_root = sh.git("rev-parse", "--show-toplevel").strip()
        codeowners = os.path.join(git_root, "CODEOWNERS")
        if not os.path.exists(codeowners):
            self.skip("CODEOWNERS not available in this repo")

        commit = sh.git("diff","--name-only", "--diff-filter=A", self.commit_range, **sh_special_args)
        new_files = commit.split("\n")
        files_in_tree = sh.git("ls-files",  **sh_special_args).split("\n")
        git = set(files_in_tree)
        if new_files:
            owned = self.parse_codeowners(git_root, codeowners)
            new_not_owned = []
            for f in new_files:
                if not f:
                    continue
                if f not in owned:
                    new_not_owned.append(f)

            if new_not_owned:
                self.add_failure("New files added that are not covered in "
                                 "CODEOWNERS:\n\n" + "\n".join(new_not_owned) +
                                 "\n\nPlease add one or more entries in the "
                                 "CODEOWNERS file to cover those files")

class Documentation(ComplianceTest):
    """
    Checks if documentation build has generated any new warnings.

    """
    _name = "Documentation"
    _doc = "https://docs.zephyrproject.org/latest/documentation/doc-guidelines.html"

    DOCS_WARNING_FILE = "doc.warnings"

    def run(self):
        self.prepare()

        if os.path.exists(self.DOCS_WARNING_FILE) and os.path.getsize(self.DOCS_WARNING_FILE) > 0:
            with open(self.DOCS_WARNING_FILE, "rb") as docs_warning:
                self.add_failure(docs_warning.read().decode("utf-8"))


class GitLint(ComplianceTest):
    """
    Runs gitlint on the commits and finds issues with style and syntax

    """
    _name = "Gitlint"
    _doc = "https://docs.zephyrproject.org/latest/contribute/#commit-guidelines"

    def run(self):
        self.prepare()

        proc = subprocess.Popen('gitlint --commits %s' % (self.commit_range),
                                shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)

        msg = ""
        if proc.wait() != 0:
            msg = proc.stdout.read()

        if msg != "":
            self.add_failure(msg.decode("utf-8"))


class License(ComplianceTest):
    """
    Checks for licenses in new files added by the Pull request

    """
    _name = "License"
    _doc = "https://docs.zephyrproject.org/latest/contribute/#licensing"

    def run(self):
        self.prepare()

        scancode = "/opt/scancode-toolkit/scancode"
        if not os.path.exists(scancode):
            self.skip("scancode-toolkit not installed")

        os.makedirs("scancode-files", exist_ok=True)
        new_files = sh.git("diff", "--name-only", "--diff-filter=A",
                           self.commit_range, **sh_special_args)

        if not new_files:
            return

        for newf in new_files:
            file = str(newf).rstrip()
            os.makedirs(os.path.join('scancode-files',
                                     os.path.dirname(file)), exist_ok=True)
            copy = os.path.join("scancode-files", file)
            copyfile(file, copy)

        try:
            cmd = [scancode, '--verbose', '--copyright', '--license', '--license-diag', '--info',
                   '--classify', '--summary', '--html', 'scancode.html', '--json', 'scancode.json', 'scancode-files/']

            cmd_str = " ".join(cmd)
            logging.info(cmd_str)

            subprocess.check_output(cmd_str, stderr=subprocess.STDOUT,
                                    shell=True)

        except subprocess.CalledProcessError as ex:
            logging.error(ex.output)
            self.error("Exception when running scancode: " + str(ex))

        report = ""

        whitelist_extensions =  ['.yaml', '.html']
        whitelist_languages = ['CMake', 'HTML']
        with open('scancode.json', 'r') as json_fp:
            scancode_results = json.load(json_fp)
            for file in scancode_results['files']:
                if file['type'] == 'directory':
                    continue

                original_fp = str(file['path']).replace('scancode-files/', '')
                licenses = file['licenses']
                if (file['is_script'] or file['is_source']) and (file['programming_language'] not in whitelist_languages) and (file['extension'] not in whitelist_extensions):
                    if not file['licenses']:
                        report += ("* {} missing license.\n".format(original_fp))
                    else:
                        for lic in licenses:
                            if lic['key'] != "apache-2.0":
                                report += ("* {} is not apache-2.0 licensed: {}\n".format(
                                    original_fp, lic['key']))
                            if lic['category'] != 'Permissive':
                                report += ("* {} has non-permissive license: {}\n".format(
                                    original_fp, lic['key']))
                            if lic['key'] == 'unknown-spdx':
                                report += ("* {} has unknown SPDX: {}\n".format(
                                    original_fp, lic['key']))

                    if not file['copyrights']:
                        report += ("* {} missing copyright.\n".format(original_fp))

        if report != "":
            self.add_failure("""
In most cases you do not need to do anything here, especially if the files
reported below are going into ext/ and if license was approved for inclusion
into ext/ already. Fix any missing license/copyright issues. The license
exception if a JFYI for the maintainers and can be overriden when merging the
pull request.\n\n""" + report)


class Identity(ComplianceTest):
    """
    Checks if Emails of author and signed-off messages are consistent.
    """
    _name = "Identity/Emails"
    _doc = "https://docs.zephyrproject.org/latest/contribute/#commit-guidelines"

    def run(self):
        self.prepare()

        for file in get_shas(self.commit_range):
            commit = sh.git("log", "--decorate=short",
                            "-n 1", file, **sh_special_args)
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

            error1 = "%s: author email (%s) needs to match one of the signed-off-by entries." % (
                sha, author)
            error2 = "%s: author email (%s) does not follow the syntax: First Last <email>." % (
                sha, author)
            failure = None
            if author not in signed:
                failure = error1

            if not parsed_addr or len(parsed_addr[0].split(" ")) < 2:
                if not failure:

                    failure = error2
                else:
                    failure = failure + "\n" + error2

            if failure:
                self.add_failure(failure)


def init_logs(cli_arg):

    """
    Initialize Logging

    :return:
    """

    # TODO: there may be a shorter version thanks to:
    # logging.basicConfig(...)

    global logger

    level = os.environ.get('LOG_LEVEL', "WARN")

    console = logging.StreamHandler()
    format = logging.Formatter('%(levelname)-8s: %(message)s')
    console.setFormatter(format)

    logger = logging.getLogger('')
    logger.addHandler(console)
    logger.setLevel(cli_arg if cli_arg else level)

    logging.info("Log init completed, level=%s",
                 logging.getLevelName(logger.getEffectiveLevel()))



def set_status(repo, sha):
    """
    Set status on Github
    :param repo:  repoistory name
    :param sha:  pull request HEAD SHA
    :return:
    """

    if 'GH_TOKEN' not in os.environ:
        return
    github_token = os.environ['GH_TOKEN']
    github_conn = Github(github_token)

    repo = github_conn.get_repo(repo)
    commit = repo.get_commit(sha)
    for testcase in ComplianceTest.__subclasses__():
        test = testcase(None, "")
        print("Creating status for %s" % (test._name))
        commit.create_status('pending',
                             '%s' % test._doc,
                             'Checks in progress',
                             '{}'.format(test._name))


def report_to_github(repo, pull_request, sha, suite, docs):
    """
    Report test results to Github

    :param repo: repo name
    :param pull_request:  pull request number
    :param sha:  pull request SHA
    :param suite:  Test suite
    :param docs:  documentation of statuses
    :return: nothing
    """

    if 'GH_TOKEN' not in os.environ:
        return

    username = os.environ.get('GH_USERNAME', 'zephyrbot')

    github_token = os.environ['GH_TOKEN']
    github_conn = Github(github_token)

    repo = github_conn.get_repo(repo)
    gh_pr = repo.get_pull(pull_request)
    commit = repo.get_commit(sha)

    comment = "Found the following issues, please fix and resubmit:\n\n"
    comment_count = 0

    print("Processing results...")

    for case in suite:
        if not case.result:
            print("reporting success on %s" %case.name)
            commit.create_status('success',
                                 docs[case.name],
                                 'Checks passed',
                                 '{}'.format(case.name))
        elif case.result.type in ['skipped']:
            print("reporting skipped on %s" %case.name)
            commit.create_status('success',
                                 docs[case.name],
                                 'Checks skipped',
                                 '{}'.format(case.name))
        elif case.result.type in ['failure']:
            print("reporting failure on %s" %case.name)
            comment_count += 1
            comment += ("## {}\n".format(case.result.message))
            comment += "\n"
            if case.name not in ['Gitlint', 'Identity/Emails', 'License']:
                comment += "```\n"
            comment += ("{}\n".format(case.result._elem.text))
            if case.name not in ['Gitlint', 'Identity/Emails', 'License']:
                comment += "```\n"

            commit.create_status('failure',
                                 docs[case.name],
                                 'Checks failed',
                                 '{}'.format(case.name))
        elif case.result.type in ['error']:
            print("reporting error on %s" %case.name)
            commit.create_status('error',
                                 docs[case.name],
                                 'Error during verification, please report!',
                                 '{}'.format(case.name))
        else:
            print("Unhandled status")


    if not repo and not pull_request:
        return comment_count

    if comment_count > 0:
        comments = gh_pr.get_issue_comments()
        commented = False
        for cmnt in comments:
            if ('Found the following issues, please fix and resubmit' in cmnt.body or
                '**All checks are passing now.**' in cmnt.body) and cmnt.user.login == username:
                if cmnt.body != comment:
                    cmnt.edit(comment)
                commented = True
                break

        if not commented:
            gh_pr.create_issue_comment(comment)
    else:
        comments = gh_pr.get_issue_comments()
        for cmnt in comments:
            if 'Found the following issues, please fix and resubmit' in cmnt.body and cmnt.user.login == username:
                cmnt.edit("**All checks are passing now.**\n\nReview history of this comment for details about previous failed status.\n"
                          "Note that some checks might have not completed yet.")
                break

    return comment_count


def parse_args():
    """
    Parse arguments
    :return:
    """
    parser = argparse.ArgumentParser(
        description="Check for coding style and documentation warnings.")
    parser.add_argument('-c', '--commits', default="HEAD~1..",
                        help='''Commit range in the form: a..[b], default is
                        HEAD~1..HEAD''')
    parser.add_argument('-g', '--github', action="store_true",
                        help="Send results to github as a comment.")

    parser.add_argument('-r', '--repo', default=None,
                        help="Github repository")
    parser.add_argument('-p', '--pull-request', default=0, type=int,
                        help="Pull request number")

    parser.add_argument('-s', '--status', action="store_true",
                        help="Set status to pending")
    parser.add_argument('-S', '--sha', default=None, help="Commit SHA")
    parser.add_argument('-o', '--output', default="compliance.xml",
                        help='''Name of outfile in JUnit format,
                        default is ./compliance.xml''')

    parser.add_argument('-l', '--list', action="store_true",
                        help="List all test modules.")

    parser.add_argument("-v", "--loglevel", help="python logging level")

    parser.add_argument('-m', '--module', action="append", default=[],
                        help="Test modules to run, by default run everything.")

    parser.add_argument('-e', '--exclude-module', action="append", default=[],
                        help="Do not run the specified modules")

    parser.add_argument('-j', '--previous-run', default=None,
                        help='''Pre-load JUnit results in XML format
                        from a previous run and combine with new results.''')


    return parser.parse_args()


def main():
    """
    Main function

    :return:
    """

    args = parse_args()

    init_logs(args.loglevel)

    if args.list:
        for testcase in ComplianceTest.__subclasses__():
            test = testcase(None, "")
            print("{}".format(test._name))
        sys.exit(0)

    if args.status and args.sha is not None and args.repo:
        set_status(args.repo, args.sha)
        sys.exit(0)

    if not args.commits:
        print("No commit range given.")
        sys.exit(1)


    if args.previous_run and os.path.exists(args.previous_run) and args.module:
        junit_xml = JUnitXml.fromfile(args.previous_run)
        logging.info("Loaded previous results from %s", args.previous_run)
        for loaded_suite in junit_xml:
            suite = loaded_suite
            break

    else:
        suite = TestSuite("Compliance")

    docs = {}
    for testcase in ComplianceTest.__subclasses__():
        test = testcase(None, "")
        docs[test._name] = test._doc


    for testcase in ComplianceTest.__subclasses__():
        test = testcase(suite, args.commits)
        if args.module:
            if test._name not in args.module:
                continue
        elif test._name in args.exclude_module:
            print("Skipping " + test._name)
            continue

        try:
            test.run()
        except EndTest:
            pass
        suite.add_testcase(test.case)

    xml = JUnitXml()
    xml.add_testsuite(suite)
    xml.update_statistics()
    xml.write(args.output, pretty=True)

    failed_cases = []

    # TODO maybe: move all the github-related code to a different .py
    # file to draw a better line between developer code versus
    # infrastructure-specific code, in other words keep this file
    # 100% testable and maintainable by non-admins developers.
    if args.github and 'GH_TOKEN' in os.environ:
        errors = report_to_github(args.repo, args.pull_request, args.sha, suite, docs)
    else:
        for case in suite:
            if case.result:
                if case.result.type == 'skipped':
                    logging.warning("Skipped %s, %s", case.name, case.result.message)
                else:
                    failed_cases.append(case)
            else:
                # Some checks like codeowners can produce no .result
                logging.info("No JUnit result for %s", case.name)

        errors = len(failed_cases)

    if errors:
        print("{} checks failed".format(errors))
        for case in failed_cases:
            # not clear why junitxml doesn't clearly expose the most
            # important part of its underlying etree.Element
            errmsg = case.result._elem.text
            errmsg = errmsg.strip() if errmsg else case.result.message
            logging.error("Test %s failed: %s", case.name, errmsg)

    print("\nComplete results in %s" % args.output)
    sys.exit(errors)

if __name__ == "__main__":
    main()
