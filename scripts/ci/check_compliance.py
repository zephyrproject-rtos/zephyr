#!/usr/bin/env python3
#
# Copyright (c) 2018 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

import collections
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
from colorama import Fore, Style
from pathlib import Path

logger = None


def info(what):
    sys.stdout.write(what + "\n")
    sys.stdout.flush()


def error(what):
    sys.stderr.write(Fore.RED + what + Style.RESET_ALL + "\n")


def git(*args):
    # Runs a git command using the 'sh' library (https://amoffat.github.io/sh/)

    # Do not create a TTY for stdout, so that Git doesn't start a pager.
    #
    # Hack: Setting _cwd to the working dir seems pointless as of writing (it
    # should be the default), but keep it around in case it's working around
    # some issue

    # pylint doesn't like how the sh library works.
    #
    # pylint: disable=unexpected-keyword-arg
    return sh.git(*args, _tty_out=False, _cwd=os.getcwd())


# This ends up as None when we're not running in a Zephyr tree
ZEPHYR_BASE = os.environ.get('ZEPHYR_BASE')

# The absolute path of the top-level git directory
GIT_TOP = git("rev-parse", "--show-toplevel").strip()


def get_shas(refspec):
    """
    Returns the list of Git SHAs for 'refspec'.

    :param refspec:
    :return:
    """
    return git('rev-list',
               '--max-count={}'.format(-1 if "." in refspec else 1),
               refspec).split()


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
        # get() defaults to None if not present

    def prepare(self, where):
        """
        Prepare test case
        :return:
        """
        self.case = MyCase(self._name)
        self.case.classname = "Guidelines"
        print("Running {:16} tests in {} ...".format(self._name, where))

    def run(self):
        """
        Run testcase
        :return:
        """

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
        self.prepare(GIT_TOP)
        # Default to Zephyr's checkpatch if ZEPHYR_BASE is set
        checkpatch = os.path.join(ZEPHYR_BASE or GIT_TOP, 'scripts',
                                  'checkpatch.pl')
        if not os.path.exists(checkpatch):
            self.skip(checkpatch + " not found")

        # git diff's output doesn't depend on the current (sub)directory
        diff = subprocess.Popen(('git', 'diff', '%s' % (self.commit_range)),
                                stdout=subprocess.PIPE)
        try:
            subprocess.check_output((checkpatch, '--mailback', '--no-tree', '-'),
                                    stdin=diff.stdout,
                                    stderr=subprocess.STDOUT,
                                    shell=True, cwd=GIT_TOP)

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
    _doc = "https://docs.zephyrproject.org/latest/guides/kconfig/index.html"

    def run(self):
        self.prepare(ZEPHYR_BASE)

        kconf = self.parse_kconfig()

        self.check_top_menu_not_too_long(kconf)
        self.check_no_undef_within_kconfig(kconf)
        self.check_no_undef_outside_kconfig(kconf)

    def get_modules(self, modules_file):
        """
        Get a list of modules and put them in a file that is parsed by
        Kconfig

        This is needed to complete Kconfig sanity tests.

        """
        # Invoke the script directly using the Python executable since this is
        # not a module nor a pip-installed Python utility
        zephyr_module_path = os.path.join(ZEPHYR_BASE, "scripts",
                                          "zephyr_module.py")
        cmd = [sys.executable, zephyr_module_path,
               '--kconfig-out', modules_file]
        try:
            _ = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
        except subprocess.CalledProcessError as ex:
            self.error(ex.output)

    def parse_kconfig(self):
        """
        Returns a kconfiglib.Kconfig object for the Kconfig files. We reuse
        this object for all tests to avoid having to reparse for each test.
        """
        if not ZEPHYR_BASE:
            self.skip("Not a Zephyr tree (ZEPHYR_BASE unset)")

        # Put the Kconfiglib path first to make sure no local Kconfiglib version is
        # used
        kconfig_path = os.path.join(ZEPHYR_BASE, "scripts", "kconfig")
        if not os.path.exists(kconfig_path):
            self.error(kconfig_path + " not found")

        sys.path.insert(0, kconfig_path)
        import kconfiglib

        # Look up Kconfig files relative to ZEPHYR_BASE
        os.environ["srctree"] = ZEPHYR_BASE

        # Parse the entire Kconfig tree, to make sure we see all symbols
        os.environ["SOC_DIR"] = "soc/"
        os.environ["ARCH_DIR"] = "arch/"
        os.environ["BOARD_DIR"] = "boards/*/*"
        os.environ["ARCH"] = "*"
        os.environ["CMAKE_BINARY_DIR"] = tempfile.gettempdir()
        os.environ['GENERATED_DTS_BOARD_CONF'] = "dummy"

        # For multi repo support
        self.get_modules(os.path.join(tempfile.gettempdir(), "Kconfig.modules"))

        # Tells Kconfiglib to generate warnings for all references to undefined
        # symbols within Kconfig files
        os.environ["KCONFIG_WARN_UNDEF"] = "y"

        try:
            # Note this will both print warnings to stderr _and_ return
            # them: so some warnings might get printed
            # twice. "warn_to_stderr=False" could unfortunately cause
            # some (other) warnings to never be printed.
            return kconfiglib.Kconfig()
        except kconfiglib.KconfigError as e:
            self.add_failure(str(e))
            raise EndTest

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

    def check_no_undef_within_kconfig(self, kconf):
        """
        Checks that there are no references to undefined Kconfig symbols within
        the Kconfig files
        """
        undef_ref_warnings = "\n\n\n".join(warning for warning in kconf.warnings
                                           if "undefined symbol" in warning)

        if undef_ref_warnings:
            self.add_failure("Undefined Kconfig symbols:\n\n"
                             + undef_ref_warnings)

    def check_no_undef_outside_kconfig(self, kconf):
        """
        Checks that there are no references to undefined Kconfig symbols
        outside Kconfig files (any CONFIG_FOO where no FOO symbol exists)
        """
        # Grep for symbol references.
        #
        # Example output line for a reference to CONFIG_BAZ at line 17 of
        # foo/bar.c:
        #
        #   foo/bar.c<null>17<null>#ifdef CONFIG_BAZ
        #
        # 'git grep --only-matching' would get rid of the surrounding context
        # ('#ifdef '), but it was added fairly recently (second half of 2018),
        # so we extract the references from each line ourselves instead.
        #
        # The regex uses word boundaries (\b) to isolate the reference, and
        # negative lookahead to automatically whitelist the following:
        #
        #  - ##, for token pasting (CONFIG_FOO_##X)
        #
        #  - $, e.g. for CMake variable expansion (CONFIG_FOO_${VAR})
        #
        #  - @, e.g. for CMakes's configure_file() (CONFIG_FOO_@VAR@)
        #
        #  - {, e.g. for Python scripts ("CONFIG_FOO_{}_BAR".format(...)")
        #
        #  - *, meant for comments like '#endif /* CONFIG_FOO_* */
        #
        # We skip the samples/ and tests/ directories for now. They often
        # contain Kconfig files that are not part of the main Kconfig tree,
        # which will trigger false positives until we do something fancier.
        #
        # We also skip doc/releases, which often references removed symbols.

        # Warning: Needs to work with both --perl-regexp and the 're' module
        regex = r"\bCONFIG_[A-Z0-9_]+\b(?!\s*##|[$@{*])"

        grep_cmd = ["git", "grep", "--line-number", "-I", "--null",
                    "--perl-regexp", regex,
                    "--", ":!samples", ":!tests", ":!doc/releases"]

        grep_process = subprocess.Popen(grep_cmd,
                                        stdout=subprocess.PIPE,
                                        stderr=subprocess.PIPE,
                                        cwd=ZEPHYR_BASE)

        grep_stdout, grep_stderr = grep_process.communicate()
        # Fail if there's anything on stderr too, so that it doesn't get missed
        if grep_process.returncode or grep_stderr:
            self.error("'{}' failed with exit code {} (while searching for "
                       "Kconfig symbol references)\n\nstdout:\n{}\n\n"
                       "stderr:\n{}"
                       .format(" ".join(grep_cmd), grep_process.returncode,
                               grep_stdout, grep_stderr))

        defined_syms = set(sym.name for sym in kconf.unique_defined_syms)
        undef_to_locs = collections.defaultdict(list)

        # splitlines() supports various line terminators
        for grep_line in grep_stdout.decode("utf-8").splitlines():
            path, lineno, line = grep_line.split("\0")

            # Extract symbol references (might be more than one) within the
            # line
            for sym_name in re.findall(regex, line):
                sym_name = sym_name[7:]  # Strip CONFIG_
                if sym_name not in defined_syms and \
                   sym_name not in UNDEF_KCONFIG_WHITELIST:

                    undef_to_locs[sym_name].append("{}:{}".format(path, lineno))

        if not undef_to_locs:
            return

        # String that describes all referenced but undefined Kconfig symbols,
        # in alphabetical order, along with the locations where they're
        # referenced. Example:
        #
        #   CONFIG_ALSO_MISSING    arch/xtensa/core/fatal.c:273
        #   CONFIG_MISSING         arch/xtensa/core/fatal.c:264, subsys/fb/cfb.c:20
        undef_desc = "\n".join(
            "CONFIG_{:35} {}".format(sym_name, ", ".join(locs))
            for sym_name, locs in sorted(undef_to_locs.items()))

        self.add_failure("""
Found references to undefined Kconfig symbols. If any of these are false
positives, then add them to UNDEF_KCONFIG_WHITELIST in {} in the ci-tools repo.

If the reference is for a comment like /* CONFIG_FOO_* */ (or
/* CONFIG_FOO_*_... */), then please use exactly that form (with the '*'). The
CI check knows not to flag it.

More generally, a reference followed by $, @, {{, *, or ## will never be
flagged.

{}""".format(os.path.basename(__file__), undef_desc))


# Many of these are symbols used as examples. Note that the list is sorted
# alphabetically, and skips the CONFIG_ prefix.
UNDEF_KCONFIG_WHITELIST = {
    "APP_LINK_WITH_",
    "CDC_ACM_PORT_NAME_",
    "CLOCK_STM32_SYSCLK_SRC_",
    "CMU",
    "COUNTER_RTC_STM32_CLOCK_SRC",
    "CRC",  # Used in TI CC13x2 / CC26x2 SDK comment
    "DEEP_SLEEP",  # #defined by RV32M1 in ext/
    "DESCRIPTION",
    "ERR",
    "ESP_DIF_LIBRARY",  # Referenced in CMake comment
    "EXPERIMENTAL",
    "FFT",  # Used as an example in cmake/extensions.cmake
    "FLAG",  # Used as an example
    "FOO",
    "FOO_LOG_LEVEL",
    "FOO_SETTING_1",
    "FOO_SETTING_2",
    "LIS2DW12_INT_PIN",
    "LSM6DSO_INT_PIN",
    "MODULES",
    "MYFEATURE",
    "MY_DRIVER_0",
    "NORMAL_SLEEP",  # #defined by RV32M1 in ext/
    "OPT",
    "OPT_0",
    "PEDO_THS_MIN",
    "REG1",
    "REG2",
    "SEL",
    "SHIFT",
    "SOC_WATCH",  # Issue 13749
    "SOME_BOOL",
    "SOME_INT",
    "SOME_OTHER_BOOL",
    "SOME_STRING",
    "STD_CPP",  # Referenced in CMake comment
    "TEST1",
    "TYPE_BOOLEAN",
    "USB_CONSOLE",
    "USE_STDC_",
    "WHATEVER",
}


class DeviceTreeCheck(ComplianceTest):
    """
    Runs the dtlib and edtlib test suites in scripts/dts/.
    """
    _name = "Device tree"
    _doc = "https://docs.zephyrproject.org/latest/guides/dts/index.html"

    def run(self):
        self.prepare(ZEPHYR_BASE)
        if not ZEPHYR_BASE:
            self.skip("Not a Zephyr tree (ZEPHYR_BASE unset)")

        scripts_path = os.path.join(ZEPHYR_BASE, "scripts", "dts")

        sys.path.insert(0, scripts_path)
        import testdtlib
        import testedtlib

        # Hack: The test suites expect to be run from the scripts/dts
        # directory, because they compare repr() output that contains relative
        # paths against an expected string. Temporarily change the working
        # directory to scripts/dts/.
        #
        # Warning: This is not thread-safe, though the test suites run in a
        # fraction of a second.
        old_dir = os.getcwd()
        os.chdir(scripts_path)
        try:
            testdtlib.run()
            testedtlib.run()
        except Exception as e:
            self.add_failure(str(e))
        finally:
            # Restore working directory
            os.chdir(old_dir)


class Codeowners(ComplianceTest):
    """
    Check if added files have an owner.
    """
    _name = "Codeowners"
    _doc  = "https://help.github.com/articles/about-code-owners/"

    def ls_owned_files(self, codeowners):
        """Returns an OrderedDict mapping git patterns from the CODEOWNERS file
        to the corresponding list of files found on the filesystem.  It
        unfortunately does not seem possible to invoke git and re-use
        how 'git ignore' and/or 'git attributes' already implement this,
        we must re-invent it.
        """

        # TODO: filter out files not in "git ls-files" (e.g.,
        # sanity-out) _if_ the overhead isn't too high for a clean tree.
        #
        # pathlib.match() doesn't support **, so it looks like we can't
        # recursively glob the output of ls-files directly, only real
        # files :-(

        pattern2files = collections.OrderedDict()
        top_path = Path(GIT_TOP)

        with open(codeowners, "r") as codeo:
            for lineno, line in enumerate(codeo, start=1):

                if line.startswith("#") or not line.strip():
                    continue

                match = re.match(r"^([^\s,]+)\s+[^\s]+", line)
                if not match:
                    self.add_failure(
                        "Invalid CODEOWNERS line %d\n\t%s" %
                        (lineno, line))
                    continue

                git_patrn = match.group(1)
                glob = self.git_pattern_to_glob(git_patrn)
                files = []
                for abs_path in top_path.glob(glob):
                    # comparing strings is much faster later
                    files.append(str(abs_path.relative_to(top_path)))

                if not files:
                    self.add_failure("Path '{}' not found in the tree but is listed in "
                                     "CODEOWNERS".format(git_patrn))

                pattern2files[git_patrn] = files

        return pattern2files

    def git_pattern_to_glob(self, git_pattern):
        """Appends and prepends '**[/*]' when needed. Result has neither a
        leading nor a trailing slash.
        """

        if git_pattern.startswith("/"):
            ret = git_pattern[1:]
        else:
            ret = "**/" + git_pattern

        if git_pattern.endswith("/"):
            ret = ret + "**/*"
        elif os.path.isdir(os.path.join(GIT_TOP, ret)):
            self.add_failure("Expected '/' after directory '{}' "
                             "in CODEOWNERS".format(ret))

        return ret

    def run(self):
        self.prepare(GIT_TOP)
        # TODO: testing an old self.commit range that doesn't end
        # with HEAD is most likely a mistake. Should warn, see
        # https://github.com/zephyrproject-rtos/ci-tools/pull/24
        codeowners = os.path.join(GIT_TOP, "CODEOWNERS")
        if not os.path.exists(codeowners):
            self.skip("CODEOWNERS not available in this repo")

        name_changes = git("diff", "--name-only", "--diff-filter=ARCD",
                            self.commit_range)

        owners_changes = git("diff", "--name-only", self.commit_range,
                             "--", codeowners)

        if not name_changes and not owners_changes:
            # TODO: 1. decouple basic and cheap CODEOWNERS syntax
            # validation from the expensive ls_owned_files() scanning of
            # the entire tree. 2. run the former always.
            return

        logging.info("If this takes too long then cleanup and try again")
        patrn2files = self.ls_owned_files(codeowners)

        # The way git finds Renames and Copies is not "exact science",
        # however if one is missed then it will always be reported as an
        # Addition instead.
        new_names = git("diff", "--name-only", "--diff-filter=ARC",
                     self.commit_range)
        new_files = new_names.splitlines()
        logging.debug("New files %s", new_files)

        # Convert to pathlib.Path string representation (e.g.,
        # backslashes 'dir1\dir2\' on Windows) to be consistent
        # with self.ls_owned_files()
        new_files = [str(Path(f)) for f in new_files]

        new_not_owned = []
        for newf in new_files:
            f_is_owned = False

            for git_pat, owned in patrn2files.items():
                logging.debug("Scanning %s for %s", git_pat, newf)

                if newf in owned:
                    logging.info("%s matches new file %s", git_pat, newf)
                    f_is_owned = True
                    # Unlike github, we don't care about finding any
                    # more specific owner.
                    break

            if not f_is_owned:
                new_not_owned.append(newf)

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
    _doc = "https://docs.zephyrproject.org/latest/guides/documentation/index.html"

    DOCS_WARNING_FILE = "doc.warnings"

    def run(self):
        self.prepare(os.getcwd())

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
        self.prepare(GIT_TOP)

        # By default gitlint looks for .gitlint configuration only in
        # the current directory
        proc = subprocess.Popen('gitlint --commits %s' % (self.commit_range),
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                shell=True, cwd=GIT_TOP)

        msg = ""
        if proc.wait() != 0:
            msg = proc.stdout.read()

        if msg != "":
            self.add_failure(msg.decode("utf-8"))


class PyLint(ComplianceTest):
    """
    Runs pylint on all .py files, with a limited set of checks enabled. The
    configuration is in the pylintrc file.
    """
    _name = "pylint"
    _doc = "https://www.pylint.org/"

    def run(self):
        self.prepare(ZEPHYR_BASE)

        # Path to pylint configuration file
        pylintrc = os.path.join(os.path.dirname(__file__), "pylintrc")

        # List of .py files added/modified by the commit(s).
        #
        # Skip create_board_img.py to work around a crash in pylint 2.2.2:
        # https://github.com/PyCQA/pylint/issues/2906
        py_files = git(
            "diff", "--name-only", "--diff-filter=d", self.commit_range,
            "--", "*.py",
            ":!boards/xtensa/intel_s1000_crb/support/create_board_img.py") \
            .splitlines()

        if not py_files:
            return

        try:
            # Run pylint on added/modified .py files
            process = subprocess.Popen(
                ["pylint", "--rcfile=" + pylintrc] + py_files,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                cwd=ZEPHYR_BASE)
        except FileNotFoundError:
            self.error("pylint not found. Check that the directory it's in is "
                       "listed in the PATH environment variable.")
        except OSError as e:
            self.error("Failed to run pylint: " + str(e))

        stdout, stderr = process.communicate()
        if process.returncode or stderr:
            # Issues found, or a problem with pylint itself
            self.add_failure(stdout.decode("utf-8") + stderr.decode("utf-8"))


class License(ComplianceTest):
    """
    Checks for licenses in new files added by the Pull request

    """
    _name = "License"
    _doc = "https://docs.zephyrproject.org/latest/contribute/#licensing"

    def run(self):
        # copyfile() below likely requires that getcwd()==GIT_TOP
        self.prepare(os.getcwd())

        scancode = "/opt/scancode-toolkit/scancode"
        if not os.path.exists(scancode):
            self.skip("scancode-toolkit not installed")

        os.makedirs("scancode-files", exist_ok=True)
        # git diff's output doesn't depend on the current (sub)directory
        new_files = git("diff", "--name-only", "--diff-filter=A",
                        self.commit_range)

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

        never_check_ext =  ['.yaml', '.html', '.rst', '.conf', '.cfg']
        never_check_langs = ['HTML']
        check_langs = ['CMake']
        with open('scancode.json', 'r') as json_fp:
            scancode_results = json.load(json_fp)
            for file in scancode_results['files']:
                if file['type'] == 'directory':
                    continue

                orig_path = str(file['path']).replace('scancode-files/', '')
                licenses = file['licenses']
                file_type = file.get("file_type")
                kconfig = "Kconfig" in orig_path and file_type in ['ASCII text']
                check = False

                if file.get("extension") in never_check_ext:
                    check = False
                elif file.get("programming_language") in never_check_langs:
                    check = False
                elif kconfig:
                    check = True
                elif file.get("programming_language") in check_langs:
                    check = True
                elif file.get("is_script"):
                    check = True
                elif file.get("is_source"):
                    check = True

                if check:
                    if not licenses:
                        report += ("* {} missing license.\n".format(orig_path))
                    else:
                        for lic in licenses:
                            if lic['key'] != "apache-2.0":
                                report += ("* {} is not apache-2.0 licensed: {}\n".format(
                                    orig_path, lic['key']))
                            if lic['category'] != 'Permissive':
                                report += ("* {} has non-permissive license: {}\n".format(
                                    orig_path, lic['key']))
                            if lic['key'] == 'unknown-spdx':
                                report += ("* {} has unknown SPDX: {}\n".format(
                                    orig_path, lic['key']))

                    if not file['copyrights'] and file.get("programming_language") != 'CMake':
                        report += ("* {} missing copyright.\n".format(orig_path))

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
        # git rev-list and git log don't depend on the current
        # (sub)directory unless explicited.
        self.prepare(GIT_TOP)

        for shaidx in get_shas(self.commit_range):
            commit = git("log", "--decorate=short", "-n 1", shaidx)
            signed = []
            author = ""
            sha = ""
            parsed_addr = None
            for line in commit.split("\n"):
                match = re.search(r"^commit\s([^\s]*)", line)
                if match:
                    sha = match.group(1)
                match = re.search(r"^Author:\s(.*)", line)
                if match:
                    author = match.group(1)
                    parsed_addr = parseaddr(author)
                match = re.search(r"signed-off-by:\s(.*)", line, re.IGNORECASE)
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
