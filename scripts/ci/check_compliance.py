#!/usr/bin/env python3

# Copyright (c) 2018,2020 Intel Corporation
# Copyright (c) 2022 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import argparse
import collections
from email.utils import parseaddr
from itertools import takewhile
import json
import logging
import os
from pathlib import Path
import platform
import re
import subprocess
import sys
import tempfile
import traceback
import shlex
import shutil
import textwrap
import unidiff

from yamllint import config, linter

from junitparser import TestCase, TestSuite, JUnitXml, Skipped, Error, Failure
import magic

from west.manifest import Manifest
from west.manifest import ManifestProject

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from get_maintainer import Maintainers, MaintainersError
import list_boards
import list_hardware

logger = None

def git(*args, cwd=None, ignore_non_zero=False):
    # Helper for running a Git command. Returns the rstrip()ed stdout output.
    # Called like git("diff"). Exits with SystemError (raised by sys.exit()) on
    # errors if 'ignore_non_zero' is set to False (default: False). 'cwd' is the
    # working directory to use (default: current directory).

    git_cmd = ("git",) + args
    try:
        cp = subprocess.run(git_cmd, capture_output=True, cwd=cwd)
    except OSError as e:
        err(f"failed to run '{cmd2str(git_cmd)}': {e}")

    if not ignore_non_zero and (cp.returncode or cp.stderr):
        err(f"'{cmd2str(git_cmd)}' exited with status {cp.returncode} and/or "
            f"wrote to stderr.\n"
            f"==stdout==\n"
            f"{cp.stdout.decode('utf-8')}\n"
            f"==stderr==\n"
            f"{cp.stderr.decode('utf-8')}\n")

    return cp.stdout.decode("utf-8").rstrip()

def get_shas(refspec):
    """
    Returns the list of Git SHAs for 'refspec'.

    :param refspec:
    :return:
    """
    return git('rev-list',
               f'--max-count={-1 if "." in refspec else 1}', refspec).split()

def get_files(filter=None, paths=None):
    filter_arg = (f'--diff-filter={filter}',) if filter else ()
    paths_arg = ('--', *paths) if paths else ()
    out = git('diff', '--name-only', *filter_arg, COMMIT_RANGE, *paths_arg)
    files = out.splitlines()
    for file in list(files):
        if not os.path.isfile(os.path.join(GIT_TOP, file)):
            # Drop submodule directories from the list.
            files.remove(file)
    return files

class FmtdFailure(Failure):
    def __init__(
        self, severity, title, file, line=None, col=None, desc="", end_line=None, end_col=None
    ):
        self.severity = severity
        self.title = title
        self.file = file
        self.line = line
        self.col = col
        self.end_line = end_line
        self.end_col = end_col
        self.desc = desc
        description = f':{desc}' if desc else ''
        msg_body = desc or title

        txt = f'\n{title}{description}\nFile:{file}' + \
              (f'\nLine:{line}' if line else '') + \
              (f'\nColumn:{col}' if col else '') + \
              (f'\nEndLine:{end_line}' if end_line else '') + \
              (f'\nEndColumn:{end_col}' if end_col else '')
        msg = f'{file}' + (f':{line}' if line else '') + f' {msg_body}'
        typ = severity.lower()

        super().__init__(msg, typ)

        self.text = txt


class ComplianceTest:
    """
    Base class for tests. Inheriting classes should have a run() method and set
    these class variables:

    name:
      Test name

    doc:
      Link to documentation related to what's being tested

    path_hint:
      The path the test runs itself in. This is just informative and used in
      the message that gets printed when running the test.

      There are two magic strings that can be used instead of a path:
      - The magic string "<zephyr-base>" can be used to refer to the
      environment variable ZEPHYR_BASE or, when missing, the calculated base of
      the zephyr tree
      - The magic string "<git-top>" refers to the top-level repository
      directory. This avoids running 'git' to find the top-level directory
      before main() runs (class variable assignments run when the 'class ...'
      statement runs). That avoids swallowing errors, because main() reports
      them to GitHub
    """
    def __init__(self):
        self.case = TestCase(type(self).name, "Guidelines")
        # This is necessary because Failure can be subclassed, but since it is
        # always restored form the element tree, the subclass is lost upon
        # restoring
        self.fmtd_failures = []

    def _result(self, res, text):
        res.text = text.rstrip()
        self.case.result += [res]

    def error(self, text, msg=None, type_="error"):
        """
        Signals a problem with running the test, with message 'msg'.

        Raises an exception internally, so you do not need to put a 'return'
        after error().
        """
        err = Error(msg or f'{type(self).name} error', type_)
        self._result(err, text)

        raise EndTest

    def skip(self, text, msg=None, type_="skip"):
        """
        Signals that the test should be skipped, with message 'msg'.

        Raises an exception internally, so you do not need to put a 'return'
        after skip().
        """
        skpd = Skipped(msg or f'{type(self).name} skipped', type_)
        self._result(skpd, text)

        raise EndTest

    def failure(self, text, msg=None, type_="failure"):
        """
        Signals that the test failed, with message 'msg'. Can be called many
        times within the same test to report multiple failures.
        """
        fail = Failure(msg or f'{type(self).name} issues', type_)
        self._result(fail, text)

    def fmtd_failure(
        self, severity, title, file, line=None, col=None, desc="", end_line=None, end_col=None
    ):
        """
        Signals that the test failed, and store the information in a formatted
        standardized manner. Can be called many times within the same test to
        report multiple failures.
        """
        fail = FmtdFailure(severity, title, file, line, col, desc, end_line, end_col)
        self._result(fail, fail.text)
        self.fmtd_failures.append(fail)

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
    name = "Checkpatch"
    doc = "See https://docs.zephyrproject.org/latest/contribute/guidelines.html#coding-style for more details."
    path_hint = "<git-top>"

    def run(self):
        checkpatch = os.path.join(ZEPHYR_BASE, 'scripts', 'checkpatch.pl')
        if not os.path.exists(checkpatch):
            self.skip(f'{checkpatch} not found')

        diff = subprocess.Popen(('git', 'diff', '--no-ext-diff', COMMIT_RANGE),
                                stdout=subprocess.PIPE,
                                cwd=GIT_TOP)
        try:
            subprocess.run((checkpatch, '--mailback', '--no-tree', '-'),
                           check=True,
                           stdin=diff.stdout,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT,
                           shell=True, cwd=GIT_TOP)

        except subprocess.CalledProcessError as ex:
            output = ex.output.decode("utf-8")
            regex = r'^\s*\S+:(\d+):\s*(ERROR|WARNING):(.+?):(.+)(?:\n|\r\n?)+' \
                    r'^\s*#(\d+):\s*FILE:\s*(.+):(\d+):'

            matches = re.findall(regex, output, re.MULTILINE)

            # add a guard here for excessive number of errors, do not try and
            # process each one of them and instead push this as one failure.
            if len(matches) > 500:
                self.failure(output)
                return

            for m in matches:
                self.fmtd_failure(m[1].lower(), m[2], m[5], m[6], col=None,
                        desc=m[3])

            # If the regex has not matched add the whole output as a failure
            if len(matches) == 0:
                self.failure(output)


class BoardYmlCheck(ComplianceTest):
    """
    Check the board.yml files
    """
    name = "BoardYml"
    doc = "Check the board.yml file format"
    path_hint = "<zephyr-base>"

    def check_board_file(self, file, vendor_prefixes):
        """Validate a single board file."""
        with open(file) as fp:
            for line_num, line in enumerate(fp.readlines(), start=1):
                if "vendor:" in line:
                    _, vnd = line.strip().split(":", 2)
                    vnd = vnd.strip()
                    if vnd not in vendor_prefixes:
                        desc = f"invalid vendor: {vnd}"
                        self.fmtd_failure("error", "BoardYml", file, line_num,
                                          desc=desc)

    def run(self):
        vendor_prefixes = ["others"]
        with open(os.path.join(ZEPHYR_BASE, "dts", "bindings", "vendor-prefixes.txt")) as fp:
            for line in fp.readlines():
                line = line.strip()
                if not line or line.startswith("#"):
                    continue
                try:
                    vendor, _ = line.split("\t", 2)
                    vendor_prefixes.append(vendor)
                except ValueError:
                    self.error(f"Invalid line in vendor-prefixes.txt:\"{line}\".")
                    self.error("Did you forget the tab character?")

        path = Path(ZEPHYR_BASE)
        for file in path.glob("**/board.yml"):
            self.check_board_file(file, vendor_prefixes)


class ClangFormatCheck(ComplianceTest):
    """
    Check if clang-format reports any issues
    """
    name = "ClangFormat"
    doc = "See https://docs.zephyrproject.org/latest/contribute/guidelines.html#clang-format for more details."
    path_hint = "<git-top>"

    def run(self):
        exe = f"clang-format-diff.{'exe' if platform.system() == 'Windows' else 'py'}"

        for file in get_files():
            if Path(file).suffix not in ['.c', '.h']:
                continue

            diff = subprocess.Popen(('git', 'diff', '-U0', '--no-color', COMMIT_RANGE, '--', file),
                                    stdout=subprocess.PIPE,
                                    cwd=GIT_TOP)
            try:
                subprocess.run((exe, '-p1'),
                               check=True,
                               stdin=diff.stdout,
                               stdout=subprocess.PIPE,
                               stderr=subprocess.STDOUT,
                               cwd=GIT_TOP)

            except subprocess.CalledProcessError as ex:
                patchset = unidiff.PatchSet.from_string(ex.output, encoding="utf-8")
                for patch in patchset:
                    for hunk in patch:
                        # Strip the before and after context
                        before = next(i for i,v in enumerate(hunk) if str(v).startswith(('-', '+')))
                        after = next(i for i,v in enumerate(reversed(hunk)) if str(v).startswith(('-', '+')))
                        msg = "".join([str(l) for l in hunk[before:-after or None]])

                        # show the hunk at the last line
                        self.fmtd_failure("notice",
                                          "You may want to run clang-format on this change",
                                          file, line=hunk.source_start + hunk.source_length - after,
                                          desc=f'\r\n{msg}')


class DevicetreeBindingsCheck(ComplianceTest):
    """
    Checks if we are introducing any unwanted properties in Devicetree Bindings.
    """
    name = "DevicetreeBindings"
    doc = "See https://docs.zephyrproject.org/latest/build/dts/bindings.html for more details."
    path_hint = "<zephyr-base>"

    def run(self, full=True):
        dts_bindings = self.parse_dt_bindings()

        for dts_binding in dts_bindings:
            self.required_false_check(dts_binding)

    def parse_dt_bindings(self):
        """
        Returns a list of dts/bindings/**/*.yaml files
        """

        dt_bindings = []
        for file_name in get_files(filter="d"):
            if 'dts/bindings/' in file_name and file_name.endswith('.yaml'):
                dt_bindings.append(file_name)

        return dt_bindings

    def required_false_check(self, dts_binding):
        with open(dts_binding) as file:
            for line_number, line in enumerate(file, 1):
                if 'required: false' in line:
                    self.fmtd_failure(
                        'warning', 'Devicetree Bindings', dts_binding,
                        line_number, col=None,
                        desc="'required: false' is redundant, please remove")


class KconfigCheck(ComplianceTest):
    """
    Checks is we are introducing any new warnings/errors with Kconfig,
    for example using undefined Kconfig variables.
    """
    name = "Kconfig"
    doc = "See https://docs.zephyrproject.org/latest/build/kconfig/tips.html for more details."
    path_hint = "<zephyr-base>"

    def run(self, full=True, no_modules=False, filename="Kconfig", hwm=None):
        self.no_modules = no_modules

        kconf = self.parse_kconfig(filename=filename, hwm=hwm)

        self.check_top_menu_not_too_long(kconf)
        self.check_no_pointless_menuconfigs(kconf)
        self.check_no_undef_within_kconfig(kconf)
        self.check_no_redefined_in_defconfig(kconf)
        self.check_no_enable_in_boolean_prompt(kconf)
        self.check_soc_name_sync(kconf)
        if full:
            self.check_no_undef_outside_kconfig(kconf)

    def get_modules(self, modules_file, settings_file):
        """
        Get a list of modules and put them in a file that is parsed by
        Kconfig

        This is needed to complete Kconfig sanity tests.

        """
        if self.no_modules:
            with open(modules_file, 'w') as fp_module_file:
                fp_module_file.write("# Empty\n")
            return

        # Invoke the script directly using the Python executable since this is
        # not a module nor a pip-installed Python utility
        zephyr_module_path = os.path.join(ZEPHYR_BASE, "scripts",
                                          "zephyr_module.py")
        cmd = [sys.executable, zephyr_module_path,
               '--kconfig-out', modules_file, '--settings-out', settings_file]
        try:
            subprocess.run(cmd, check=True, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT)
        except subprocess.CalledProcessError as ex:
            self.error(ex.output.decode("utf-8"))

        modules_dir = ZEPHYR_BASE + '/modules'
        modules = [name for name in os.listdir(modules_dir) if
                   os.path.exists(os.path.join(modules_dir, name, 'Kconfig'))]

        with open(modules_file, 'r') as fp_module_file:
            content = fp_module_file.read()

        with open(modules_file, 'w') as fp_module_file:
            for module in modules:
                fp_module_file.write("ZEPHYR_{}_KCONFIG = {}\n".format(
                    re.sub('[^a-zA-Z0-9]', '_', module).upper(),
                    modules_dir + '/' + module + '/Kconfig'
                ))
            fp_module_file.write(content)

    def get_module_setting_root(self, root, settings_file):
        """
        Parse the Zephyr module generated settings file given by 'settings_file'
        and return all root settings defined by 'root'.
        """
        # Invoke the script directly using the Python executable since this is
        # not a module nor a pip-installed Python utility
        root_paths = []

        if os.path.exists(settings_file):
            with open(settings_file, 'r') as fp_setting_file:
                content = fp_setting_file.read()

            lines = content.strip().split('\n')
            for line in lines:
                root = root.upper()
                if line.startswith(f'"{root}_ROOT":'):
                    _, root_path = line.split(":", 1)
                    root_paths.append(Path(root_path.strip('"')))
        return root_paths

    def get_kconfig_dts(self, kconfig_dts_file, settings_file):
        """
        Generate the Kconfig.dts using dts/bindings as the source.

        This is needed to complete Kconfig compliance tests.

        """
        # Invoke the script directly using the Python executable since this is
        # not a module nor a pip-installed Python utility
        zephyr_drv_kconfig_path = os.path.join(ZEPHYR_BASE, "scripts", "dts",
                                               "gen_driver_kconfig_dts.py")
        binding_paths = []
        binding_paths.append(os.path.join(ZEPHYR_BASE, "dts", "bindings"))

        dts_root_paths = self.get_module_setting_root('dts', settings_file)
        for p in dts_root_paths:
            binding_paths.append(p / "dts" / "bindings")

        cmd = [sys.executable, zephyr_drv_kconfig_path,
               '--kconfig-out', kconfig_dts_file, '--bindings-dirs']
        for binding_path in binding_paths:
            cmd.append(binding_path)
        try:
            subprocess.run(cmd, check=True, stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT)
        except subprocess.CalledProcessError as ex:
            self.error(ex.output.decode("utf-8"))

    def get_v1_model_syms(self, kconfig_v1_file, kconfig_v1_syms_file):
        """
        Generate a symbol define Kconfig file.
        This function creates a file with all Kconfig symbol definitions from
        old boards model so that those symbols will not appear as undefined
        symbols in hardware model v2.

        This is needed to complete Kconfig compliance tests.
        """
        os.environ['HWM_SCHEME'] = 'v1'
        # 'kconfiglib' is global
        # pylint: disable=undefined-variable

        try:
            kconf_v1 = kconfiglib.Kconfig(filename=kconfig_v1_file, warn=False)
        except kconfiglib.KconfigError as e:
            self.failure(str(e))
            raise EndTest

        with open(kconfig_v1_syms_file, 'w') as fp_kconfig_v1_syms_file:
            for s in kconf_v1.defined_syms:
                if s.type != kconfiglib.UNKNOWN:
                    fp_kconfig_v1_syms_file.write('config ' + s.name)
                    fp_kconfig_v1_syms_file.write('\n\t' + kconfiglib.TYPE_TO_STR[s.type])
                    fp_kconfig_v1_syms_file.write('\n\n')

    def get_v2_model(self, kconfig_dir, settings_file):
        """
        Get lists of v2 boards and SoCs and put them in a file that is parsed by
        Kconfig

        This is needed to complete Kconfig sanity tests.
        """
        os.environ['HWM_SCHEME'] = 'v2'
        kconfig_file = os.path.join(kconfig_dir, 'boards', 'Kconfig')
        kconfig_boards_file = os.path.join(kconfig_dir, 'boards', 'Kconfig.boards')
        kconfig_defconfig_file = os.path.join(kconfig_dir, 'boards', 'Kconfig.defconfig')

        board_roots = self.get_module_setting_root('board', settings_file)
        board_roots.insert(0, Path(ZEPHYR_BASE))
        soc_roots = self.get_module_setting_root('soc', settings_file)
        soc_roots.insert(0, Path(ZEPHYR_BASE))
        root_args = argparse.Namespace(**{'board_roots': board_roots,
                                          'soc_roots': soc_roots, 'board': None,
                                          'board_dir': []})
        v2_boards = list_boards.find_v2_boards(root_args).values()

        with open(kconfig_defconfig_file, 'w') as fp:
            for board in v2_boards:
                fp.write('osource "' + (Path(board.dir) / 'Kconfig.defconfig').as_posix() + '"\n')

        with open(kconfig_boards_file, 'w') as fp:
            for board in v2_boards:
                board_str = 'BOARD_' + re.sub(r"[^a-zA-Z0-9_]", "_", board.name).upper()
                fp.write('config  ' + board_str + '\n')
                fp.write('\t bool\n')
                for qualifier in list_boards.board_v2_qualifiers(board):
                    board_str = ('BOARD_' + board.name + '_' +
                                 re.sub(r"[^a-zA-Z0-9_]", "_", qualifier)).upper()
                    fp.write('config  ' + board_str + '\n')
                    fp.write('\t bool\n')
                fp.write(
                    'source "' + (Path(board.dir) / ('Kconfig.' + board.name)).as_posix() + '"\n\n'
                )

        with open(kconfig_file, 'w') as fp:
            fp.write(
                'osource "' + (Path(kconfig_dir) / 'boards' / 'Kconfig.syms.v1').as_posix() + '"\n'
            )
            for board in v2_boards:
                fp.write('osource "' + (Path(board.dir) / 'Kconfig').as_posix() + '"\n')

        kconfig_defconfig_file = os.path.join(kconfig_dir, 'soc', 'Kconfig.defconfig')
        kconfig_soc_file = os.path.join(kconfig_dir, 'soc', 'Kconfig.soc')
        kconfig_file = os.path.join(kconfig_dir, 'soc', 'Kconfig')

        root_args = argparse.Namespace(**{'soc_roots': soc_roots})
        v2_systems = list_hardware.find_v2_systems(root_args)

        soc_folders = {soc.folder[0] for soc in v2_systems.get_socs()}
        with open(kconfig_defconfig_file, 'w') as fp:
            for folder in soc_folders:
                fp.write('osource "' + (Path(folder) / 'Kconfig.defconfig').as_posix() + '"\n')

        with open(kconfig_soc_file, 'w') as fp:
            for folder in soc_folders:
                fp.write('source "' + (Path(folder) / 'Kconfig.soc').as_posix() + '"\n')

        with open(kconfig_file, 'w') as fp:
            for folder in soc_folders:
                fp.write('source "' + (Path(folder) / 'Kconfig').as_posix() + '"\n')

        kconfig_file = os.path.join(kconfig_dir, 'arch', 'Kconfig')

        root_args = argparse.Namespace(**{'arch_roots': [Path(ZEPHYR_BASE)], 'arch': None})
        v2_archs = list_hardware.find_v2_archs(root_args)

        with open(kconfig_file, 'w') as fp:
            for arch in v2_archs['archs']:
                fp.write('source "' + (Path(arch['path']) / 'Kconfig').as_posix() + '"\n')

    def parse_kconfig(self, filename="Kconfig", hwm=None):
        """
        Returns a kconfiglib.Kconfig object for the Kconfig files. We reuse
        this object for all tests to avoid having to reparse for each test.
        """
        # Put the Kconfiglib path first to make sure no local Kconfiglib version is
        # used
        kconfig_path = os.path.join(ZEPHYR_BASE, "scripts", "kconfig")
        if not os.path.exists(kconfig_path):
            self.error(kconfig_path + " not found")

        kconfiglib_dir = tempfile.mkdtemp(prefix="kconfiglib_")

        sys.path.insert(0, kconfig_path)
        # Import globally so that e.g. kconfiglib.Symbol can be referenced in
        # tests
        global kconfiglib
        import kconfiglib

        # Look up Kconfig files relative to ZEPHYR_BASE
        os.environ["srctree"] = ZEPHYR_BASE

        # Parse the entire Kconfig tree, to make sure we see all symbols
        os.environ["SOC_DIR"] = "soc/"
        os.environ["ARCH_DIR"] = "arch/"
        os.environ["BOARD"] = "boards"
        os.environ["ARCH"] = "*"
        os.environ["KCONFIG_BINARY_DIR"] = kconfiglib_dir
        os.environ['DEVICETREE_CONF'] = "dummy"
        os.environ['TOOLCHAIN_HAS_NEWLIB'] = "y"

        # Older name for DEVICETREE_CONF, for compatibility with older Zephyr
        # versions that don't have the renaming
        os.environ["GENERATED_DTS_BOARD_CONF"] = "dummy"

        # For multi repo support
        self.get_modules(os.path.join(kconfiglib_dir, "Kconfig.modules"),
                         os.path.join(kconfiglib_dir, "settings_file.txt"))
        # For Kconfig.dts support
        self.get_kconfig_dts(os.path.join(kconfiglib_dir, "Kconfig.dts"),
                             os.path.join(kconfiglib_dir, "settings_file.txt"))

        # To make compliance work with old hw model and HWMv2 simultaneously.
        kconfiglib_boards_dir = os.path.join(kconfiglib_dir, 'boards')
        os.makedirs(kconfiglib_boards_dir, exist_ok=True)
        os.makedirs(os.path.join(kconfiglib_dir, 'soc'), exist_ok=True)
        os.makedirs(os.path.join(kconfiglib_dir, 'arch'), exist_ok=True)

        os.environ["KCONFIG_BOARD_DIR"] = kconfiglib_boards_dir
        self.get_v2_model(kconfiglib_dir, os.path.join(kconfiglib_dir, "settings_file.txt"))

        # Tells Kconfiglib to generate warnings for all references to undefined
        # symbols within Kconfig files
        os.environ["KCONFIG_WARN_UNDEF"] = "y"

        try:
            # Note this will both print warnings to stderr _and_ return
            # them: so some warnings might get printed
            # twice. "warn_to_stderr=False" could unfortunately cause
            # some (other) warnings to never be printed.
            return kconfiglib.Kconfig(filename=filename)
        except kconfiglib.KconfigError as e:
            self.failure(str(e))
            raise EndTest
        finally:
            # Clean up the temporary directory
            shutil.rmtree(kconfiglib_dir)

    def get_logging_syms(self, kconf):
        # Returns a set() with the names of the Kconfig symbols generated with
        # logging template in samples/tests folders. The Kconfig symbols doesn't
        # include `CONFIG_` and for each module declared there is one symbol
        # per suffix created.

        suffixes = [
            "_LOG_LEVEL",
            "_LOG_LEVEL_DBG",
            "_LOG_LEVEL_ERR",
            "_LOG_LEVEL_INF",
            "_LOG_LEVEL_WRN",
            "_LOG_LEVEL_OFF",
            "_LOG_LEVEL_INHERIT",
            "_LOG_LEVEL_DEFAULT",
        ]

        # Warning: Needs to work with both --perl-regexp and the 're' module.
        regex = r"^\s*(?:module\s*=\s*)([A-Z0-9_]+)\s*(?:#|$)"

        # Grep samples/ and tests/ for symbol definitions
        grep_stdout = git("grep", "-I", "-h", "--perl-regexp", regex, "--",
                          ":samples", ":tests", cwd=ZEPHYR_BASE)

        names = re.findall(regex, grep_stdout, re.MULTILINE)

        kconf_syms = []
        for name in names:
            for suffix in suffixes:
                kconf_syms.append(f"{name}{suffix}")

        return set(kconf_syms)

    def get_defined_syms(self, kconf):
        # Returns a set() with the names of all defined Kconfig symbols (with no
        # 'CONFIG_' prefix). This is complicated by samples and tests defining
        # their own Kconfig trees. For those, just grep for 'config FOO' to find
        # definitions. Doing it "properly" with Kconfiglib is still useful for
        # the main tree, because some symbols are defined using preprocessor
        # macros.

        # Warning: Needs to work with both --perl-regexp and the 're' module.
        # (?:...) is a non-capturing group.
        regex = r"^\s*(?:menu)?config\s*([A-Z0-9_]+)\s*(?:#|$)"

        # Grep samples/ and tests/ for symbol definitions
        grep_stdout = git("grep", "-I", "-h", "--perl-regexp", regex, "--",
                          ":samples", ":tests", cwd=ZEPHYR_BASE)

        # Generate combined list of configs and choices from the main Kconfig tree.
        kconf_syms = kconf.unique_defined_syms + kconf.unique_choices

        # Symbols from the main Kconfig tree + grepped definitions from samples
        # and tests
        return set(
            [sym.name for sym in kconf_syms]
            + re.findall(regex, grep_stdout, re.MULTILINE)
        ).union(self.get_logging_syms(kconf))

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
            self.failure(f"""
Expected no more than {max_top_items} potentially visible items (items with
prompts) in the top-level Kconfig menu, found {n_top_items} items. If you're
deliberately adding new entries, then bump the 'max_top_items' variable in
{__file__}.""")

    def check_no_redefined_in_defconfig(self, kconf):
        # Checks that no symbols are (re)defined in defconfigs.

        for node in kconf.node_iter():
            # 'kconfiglib' is global
            # pylint: disable=undefined-variable
            if "defconfig" in node.filename and (node.prompt or node.help):
                name = (node.item.name if node.item not in
                        (kconfiglib.MENU, kconfiglib.COMMENT) else str(node))
                self.failure(f"""
Kconfig node '{name}' found with prompt or help in {node.filename}.
Options must not be defined in defconfig files.
""")
                continue

    def check_no_enable_in_boolean_prompt(self, kconf):
        # Checks that boolean's prompt does not start with "Enable...".

        for node in kconf.node_iter():
            # skip Kconfig nodes not in-tree (will present an absolute path)
            if os.path.isabs(node.filename):
                continue

            # 'kconfiglib' is global
            # pylint: disable=undefined-variable

            # only process boolean symbols with a prompt
            if (not isinstance(node.item, kconfiglib.Symbol) or
                node.item.type != kconfiglib.BOOL or
                not node.prompt or
                not node.prompt[0]):
                continue

            if re.match(r"^[Ee]nable.*", node.prompt[0]):
                self.failure(f"""
Boolean option '{node.item.name}' prompt must not start with 'Enable...'. Please
check Kconfig guidelines.
""")
                continue

    def check_no_pointless_menuconfigs(self, kconf):
        # Checks that there are no pointless 'menuconfig' symbols without
        # children in the Kconfig files

        bad_mconfs = []
        for node in kconf.node_iter():
            # 'kconfiglib' is global
            # pylint: disable=undefined-variable

            # Avoid flagging empty regular menus and choices, in case people do
            # something with 'osource' (could happen for 'menuconfig' symbols
            # too, though it's less likely)
            if node.is_menuconfig and not node.list and \
               isinstance(node.item, kconfiglib.Symbol):

                bad_mconfs.append(node)

        if bad_mconfs:
            self.failure("""\
Found pointless 'menuconfig' symbols without children. Use regular 'config'
symbols instead. See
https://docs.zephyrproject.org/latest/build/kconfig/tips.html#menuconfig-symbols.

""" + "\n".join(f"{node.item.name:35} {node.filename}:{node.linenr}"
                for node in bad_mconfs))

    def check_no_undef_within_kconfig(self, kconf):
        """
        Checks that there are no references to undefined Kconfig symbols within
        the Kconfig files
        """
        undef_ref_warnings = "\n\n\n".join(warning for warning in kconf.warnings
                                           if "undefined symbol" in warning)

        if undef_ref_warnings:
            self.failure(f"Undefined Kconfig symbols:\n\n {undef_ref_warnings}")

    def check_soc_name_sync(self, kconf):
        root_args = argparse.Namespace(**{'soc_roots': [Path(ZEPHYR_BASE)]})
        v2_systems = list_hardware.find_v2_systems(root_args)

        soc_names = {soc.name for soc in v2_systems.get_socs()}

        soc_kconfig_names = set()
        for node in kconf.node_iter():
            # 'kconfiglib' is global
            # pylint: disable=undefined-variable
            if isinstance(node.item, kconfiglib.Symbol) and node.item.name == "SOC":
                n = node.item
                for d in n.defaults:
                    soc_kconfig_names.add(d[0].name)

        soc_name_warnings = []
        for name in soc_names:
            if name not in soc_kconfig_names:
                soc_name_warnings.append(f"soc name: {name} not found in CONFIG_SOC defaults.")

        if soc_name_warnings:
            soc_name_warning_str = '\n'.join(soc_name_warnings)
            self.failure(f'''
Missing SoC names or CONFIG_SOC vs soc.yml out of sync:

{soc_name_warning_str}
''')

    def check_no_undef_outside_kconfig(self, kconf):
        """
        Checks that there are no references to undefined Kconfig symbols
        outside Kconfig files (any CONFIG_FOO where no FOO symbol exists)
        """
        # Grep for symbol references.
        #
        # Example output line for a reference to CONFIG_FOO at line 17 of
        # foo/bar.c:
        #
        #   foo/bar.c<null>17<null>#ifdef CONFIG_FOO
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

        defined_syms = self.get_defined_syms(kconf)

        # Maps each undefined symbol to a list <filename>:<linenr> strings
        undef_to_locs = collections.defaultdict(list)

        # Warning: Needs to work with both --perl-regexp and the 're' module
        regex = r"\bCONFIG_[A-Z0-9_]+\b(?!\s*##|[$@{(.*])"

        # Skip doc/releases and doc/security/vulnerabilities.rst, which often
        # reference removed symbols
        grep_stdout = git("grep", "--line-number", "-I", "--null",
                          "--perl-regexp", regex, "--", ":!/doc/releases",
                          ":!/doc/security/vulnerabilities.rst",
                          cwd=Path(GIT_TOP))

        # splitlines() supports various line terminators
        for grep_line in grep_stdout.splitlines():
            path, lineno, line = grep_line.split("\0")

            # Extract symbol references (might be more than one) within the
            # line
            for sym_name in re.findall(regex, line):
                sym_name = sym_name[7:]  # Strip CONFIG_
                if sym_name not in defined_syms and \
                   sym_name not in self.UNDEF_KCONFIG_ALLOWLIST and \
                   not (sym_name.endswith("_MODULE") and sym_name[:-7] in defined_syms):

                    undef_to_locs[sym_name].append(f"{path}:{lineno}")

        if not undef_to_locs:
            return

        # String that describes all referenced but undefined Kconfig symbols,
        # in alphabetical order, along with the locations where they're
        # referenced. Example:
        #
        #   CONFIG_ALSO_MISSING    arch/xtensa/core/fatal.c:273
        #   CONFIG_MISSING         arch/xtensa/core/fatal.c:264, subsys/fb/cfb.c:20
        undef_desc = "\n".join(f"CONFIG_{sym_name:35} {', '.join(locs)}"
            for sym_name, locs in sorted(undef_to_locs.items()))

        self.failure(f"""
Found references to undefined Kconfig symbols. If any of these are false
positives, then add them to UNDEF_KCONFIG_ALLOWLIST in {__file__}.

If the reference is for a comment like /* CONFIG_FOO_* */ (or
/* CONFIG_FOO_*_... */), then please use exactly that form (with the '*'). The
CI check knows not to flag it.

More generally, a reference followed by $, @, {{, (, ., *, or ## will never be
flagged.

{undef_desc}""")

    # Many of these are symbols used as examples. Note that the list is sorted
    # alphabetically, and skips the CONFIG_ prefix.
    UNDEF_KCONFIG_ALLOWLIST = {
        # zephyr-keep-sorted-start re(^\s+")
        "ALSO_MISSING",
        "APP_LINK_WITH_",
        "APP_LOG_LEVEL", # Application log level is not detected correctly as
                         # the option is defined using a template, so it can't
                         # be grepped
        "APP_LOG_LEVEL_DBG",
        "ARMCLANG_STD_LIBC",  # The ARMCLANG_STD_LIBC is defined in the
                              # toolchain Kconfig which is sourced based on
                              # Zephyr toolchain variant and therefore not
                              # visible to compliance.
        "BINDESC_", # Used in documentation as a prefix
        "BOARD_", # Used as regex in scripts/utils/board_v1_to_v2.py
        "BOARD_MPS2_AN521_CPUTEST", # Used for board and SoC extension feature tests
        "BOARD_NATIVE_SIM_NATIVE_64_TWO", # Used for board and SoC extension feature tests
        "BOARD_NATIVE_SIM_NATIVE_ONE", # Used for board and SoC extension feature tests
        "BOOT_DIRECT_XIP", # Used in sysbuild for MCUboot configuration
        "BOOT_DIRECT_XIP_REVERT", # Used in sysbuild for MCUboot configuration
        "BOOT_ENCRYPTION_KEY_FILE", # Used in sysbuild
        "BOOT_ENCRYPT_IMAGE", # Used in sysbuild
        "BOOT_FIRMWARE_LOADER", # Used in sysbuild for MCUboot configuration
        "BOOT_MAX_IMG_SECTORS_AUTO", # Used in sysbuild
        "BOOT_RAM_LOAD", # Used in sysbuild for MCUboot configuration
        "BOOT_SERIAL_BOOT_MODE",     # Used in (sysbuild-based) test/
                                     # documentation
        "BOOT_SERIAL_CDC_ACM",       # Used in (sysbuild-based) test
        "BOOT_SERIAL_ENTRANCE_GPIO", # Used in (sysbuild-based) test
        "BOOT_SERIAL_IMG_GRP_HASH",  # Used in documentation
        "BOOT_SHARE_BACKEND_RETENTION", # Used in Kconfig text
        "BOOT_SHARE_DATA",           # Used in Kconfig text
        "BOOT_SHARE_DATA_BOOTINFO", # Used in (sysbuild-based) test
        "BOOT_SIGNATURE_KEY_FILE",   # MCUboot setting used by sysbuild
        "BOOT_SIGNATURE_TYPE_ECDSA_P256", # MCUboot setting used by sysbuild
        "BOOT_SIGNATURE_TYPE_ED25519",    # MCUboot setting used by sysbuild
        "BOOT_SIGNATURE_TYPE_NONE",       # MCUboot setting used by sysbuild
        "BOOT_SIGNATURE_TYPE_RSA",        # MCUboot setting used by sysbuild
        "BOOT_SWAP_USING_MOVE", # Used in sysbuild for MCUboot configuration
        "BOOT_SWAP_USING_SCRATCH", # Used in sysbuild for MCUboot configuration
        "BOOT_UPGRADE_ONLY", # Used in example adjusting MCUboot config, but
                             # symbol is defined in MCUboot itself.
        "BOOT_VALIDATE_SLOT0",       # Used in (sysbuild-based) test
        "BOOT_WATCHDOG_FEED",        # Used in (sysbuild-based) test
        "BT_6LOWPAN",  # Defined in Linux, mentioned in docs
        "CDC_ACM_PORT_NAME_",
        "CHRE",  # Optional module
        "CHRE_LOG_LEVEL_DBG",  # Optional module
        "CLOCK_STM32_SYSCLK_SRC_",
        "CMD_CACHE",  # Defined in U-Boot, mentioned in docs
        "CMU",
        "COMPILER_RT_RTLIB",
        "CRC",  # Used in TI CC13x2 / CC26x2 SDK comment
        "DEEP_SLEEP",  # #defined by RV32M1 in ext/
        "DESCRIPTION",
        "ERR",
        "ESP_DIF_LIBRARY",  # Referenced in CMake comment
        "EXPERIMENTAL",
        "EXTRA_FIRMWARE_DIR", # Linux, in boards/xtensa/intel_adsp_cavs25/doc
        "FFT",  # Used as an example in cmake/extensions.cmake
        "FLAG",  # Used as an example
        "FOO",
        "FOO_LOG_LEVEL",
        "FOO_SETTING_1",
        "FOO_SETTING_2",
        "HEAP_MEM_POOL_ADD_SIZE_", # Used as an option matching prefix
        "HUGETLBFS",          # Linux, in boards/xtensa/intel_adsp_cavs25/doc
        "LIBGCC_RTLIB",
        "LLVM_USE_LD",   # Both LLVM_USE_* are in cmake/toolchain/llvm/Kconfig
        "LLVM_USE_LLD",  # which are only included if LLVM is selected but
                         # not other toolchains. Compliance check would complain,
                         # for example, if you are using GCC.
        "LOG_BACKEND_MOCK_OUTPUT_DEFAULT", #Referenced in tests/subsys/logging/log_syst
        "LOG_BACKEND_MOCK_OUTPUT_SYST", #Referenced in testcase.yaml of log_syst test
        "LSM6DSO_INT_PIN",
        "MCUBOOT_ACTION_HOOKS",     # Used in (sysbuild-based) test
        "MCUBOOT_CLEANUP_ARM_CORE", # Used in (sysbuild-based) test
        "MCUBOOT_DOWNGRADE_PREVENTION", # but symbols are defined in MCUboot
                                        # itself.
        "MCUBOOT_LOG_LEVEL_INF",
        "MCUBOOT_LOG_LEVEL_WRN",        # Used in example adjusting MCUboot
                                        # config,
        "MCUBOOT_SERIAL",           # Used in (sysbuild-based) test/
                                    # documentation
        "MCUMGR_GRP_EXAMPLE_OTHER_HOOK", # Used in documentation
        "MISSING",
        "MODULES",
        "MODVERSIONS",        # Linux, in boards/xtensa/intel_adsp_cavs25/doc
        "MYFEATURE",
        "MY_DRIVER_0",
        "NORMAL_SLEEP",  # #defined by RV32M1 in ext/
        "NRF_WIFI_FW_BIN", # Directly passed from CMakeLists.txt
        "OPT",
        "OPT_0",
        "PEDO_THS_MIN",
        "PSA_H", # This is used in config-psa.h as guard for the header file
        "REG1",
        "REG2",
        "RIMAGE_SIGNING_SCHEMA",  # Optional module
        "SECURITY_LOADPIN",   # Linux, in boards/xtensa/intel_adsp_cavs25/doc
        "SEL",
        "SHIFT",
        "SINGLE_APPLICATION_SLOT", # Used in sysbuild for MCUboot configuration
        "SOC_SERIES_", # Used as regex in scripts/utils/board_v1_to_v2.py
        "SOC_WATCH",  # Issue 13749
        "SOME_BOOL",
        "SOME_INT",
        "SOME_OTHER_BOOL",
        "SOME_STRING",
        "SRAM2",  # Referenced in a comment in samples/application_development
        "STACK_SIZE",  # Used as an example in the Kconfig docs
        "STD_CPP",  # Referenced in CMake comment
        "SUIT_MPI_APP_AREA_PATH", # Used by nRF runners to program provisioning data, based on build configuration
        "SUIT_MPI_GENERATE", # Used by nRF runners to program provisioning data, based on build configuration
        "SUIT_MPI_RAD_AREA_PATH", # Used by nRF runners to program provisioning data, based on build configuration
        "TEST1",
        "TOOLCHAIN_ARCMWDT_SUPPORTS_THREAD_LOCAL_STORAGE", # The symbol is defined in the toolchain
                                                    # Kconfig which is sourced based on Zephyr
                                                    # toolchain variant and therefore not visible
                                                    # to compliance.
        "TYPE_BOOLEAN",
        "USB_CONSOLE",
        "USE_STDC_",
        "WHATEVER",
        "ZEPHYR_TRY_MASS_ERASE", # MCUBoot setting described in sysbuild
                                 # documentation
        "ZTEST_FAIL_TEST_",  # regex in tests/ztest/fail/CMakeLists.txt
        # zephyr-keep-sorted-stop
    }


class KconfigBasicCheck(KconfigCheck):
    """
    Checks if we are introducing any new warnings/errors with Kconfig,
    for example using undefined Kconfig variables.
    This runs the basic Kconfig test, which is checking only for undefined
    references inside the Kconfig tree.
    """
    name = "KconfigBasic"
    doc = "See https://docs.zephyrproject.org/latest/build/kconfig/tips.html for more details."
    path_hint = "<zephyr-base>"

    def run(self):
        super().run(full=False)

class KconfigBasicNoModulesCheck(KconfigCheck):
    """
    Checks if we are introducing any new warnings/errors with Kconfig when no
    modules are available. Catches symbols used in the main repository but
    defined only in a module.
    """
    name = "KconfigBasicNoModules"
    doc = "See https://docs.zephyrproject.org/latest/build/kconfig/tips.html for more details."
    path_hint = "<zephyr-base>"
    def run(self):
        super().run(full=False, no_modules=True)


class KconfigHWMv2Check(KconfigCheck, ComplianceTest):
    """
    This runs the Kconfig test for board and SoC v2 scheme.
    This check ensures that all symbols inside the v2 scheme is also defined
    within the same tree.
    This ensures the board and SoC trees are fully self-contained and reusable.
    """
    name = "KconfigHWMv2"
    doc = "See https://docs.zephyrproject.org/latest/guides/kconfig/index.html for more details."

    def run(self):
        # Use dedicated Kconfig board / soc v2 scheme file.
        # This file sources only v2 scheme tree.
        kconfig_file = os.path.join(os.path.dirname(__file__), "Kconfig.board.v2")
        super().run(full=False, hwm="v2", filename=kconfig_file)


class Nits(ComplianceTest):
    """
    Checks various nits in added/modified files. Doesn't check stuff that's
    already covered by e.g. checkpatch.pl and pylint.
    """
    name = "Nits"
    doc = "See https://docs.zephyrproject.org/latest/contribute/guidelines.html#coding-style for more details."
    path_hint = "<git-top>"

    def run(self):
        # Loop through added/modified files
        for fname in get_files(filter="d"):
            if "Kconfig" in fname:
                self.check_kconfig_header(fname)
                self.check_redundant_zephyr_source(fname)

            if fname.startswith("dts/bindings/"):
                self.check_redundant_document_separator(fname)

            if fname.endswith((".c", ".conf", ".cpp", ".dts", ".overlay",
                               ".h", ".ld", ".py", ".rst", ".txt", ".yaml",
                               ".yml")) or \
               "Kconfig" in fname or \
               "defconfig" in fname or \
               fname == "README":

                self.check_source_file(fname)

    def check_kconfig_header(self, fname):
        # Checks for a spammy copy-pasted header format

        with open(os.path.join(GIT_TOP, fname), encoding="utf-8") as f:
            contents = f.read()

        # 'Kconfig - yada yada' has a copy-pasted redundant filename at the
        # top. This probably means all of the header was copy-pasted.
        if re.match(r"\s*#\s*(K|k)config[\w.-]*\s*-", contents):
            self.failure(f"""
Please use this format for the header in '{fname}' (see
https://docs.zephyrproject.org/latest/build/kconfig/tips.html#header-comments-and-other-nits):

    # <Overview of symbols defined in the file, preferably in plain English>
    (Blank line)
    # Copyright (c) 2019 ...
    # SPDX-License-Identifier: <License>
    (Blank line)
    (Kconfig definitions)

Skip the "Kconfig - " part of the first line, since it's clear that the comment
is about Kconfig from context. The "# Kconfig - " is what triggers this
failure.
""")

    def check_redundant_zephyr_source(self, fname):
        # Checks for 'source "$(ZEPHYR_BASE)/Kconfig[.zephyr]"', which can be
        # be simplified to 'source "Kconfig[.zephyr]"'

        with open(os.path.join(GIT_TOP, fname), encoding="utf-8") as f:
            # Look for e.g. rsource as well, for completeness
            match = re.search(
                r'^\s*(?:o|r|or)?source\s*"\$\(?ZEPHYR_BASE\)?/(Kconfig(?:\.zephyr)?)"',
                f.read(), re.MULTILINE)

            if match:
                self.failure("""
Redundant 'source "$(ZEPHYR_BASE)/{0}" in '{1}'. Just do 'source "{0}"'
instead. The $srctree environment variable already points to the Zephyr root,
and all 'source's are relative to it.""".format(match.group(1), fname))

    def check_redundant_document_separator(self, fname):
        # Looks for redundant '...' document separators in bindings

        with open(os.path.join(GIT_TOP, fname), encoding="utf-8") as f:
            if re.search(r"^\.\.\.", f.read(), re.MULTILINE):
                self.failure(f"""\
Redundant '...' document separator in {fname}. Binding YAML files are never
concatenated together, so no document separators are needed.""")

    def check_source_file(self, fname):
        # Generic nits related to various source files

        with open(os.path.join(GIT_TOP, fname), encoding="utf-8") as f:
            contents = f.read()

        if not contents.endswith("\n"):
            self.failure(f"Missing newline at end of '{fname}'. Check your text "
                         f"editor settings.")

        if contents.startswith("\n"):
            self.failure(f"Please remove blank lines at start of '{fname}'")

        if contents.endswith("\n\n"):
            self.failure(f"Please remove blank lines at end of '{fname}'")


class GitDiffCheck(ComplianceTest):
    """
    Checks for conflict markers or whitespace errors with git diff --check
    """
    name = "GitDiffCheck"
    doc = "Git conflict markers and whitespace errors are not allowed in added changes"
    path_hint = "<git-top>"

    def run(self):
        offending_lines = []
        # Use regex to filter out unnecessay output
        # Reason: `--check` is mutually exclusive with `--name-only` and `-s`
        p = re.compile(r"\S+\: .*\.")

        for shaidx in get_shas(COMMIT_RANGE):
            # Ignore non-zero return status code
            # Reason: `git diff --check` sets the return code to the number of offending lines
            diff = git("diff", f"{shaidx}^!", "--check", ignore_non_zero=True)

            lines = p.findall(diff)
            lines = map(lambda x: f"{shaidx}: {x}", lines)
            offending_lines.extend(lines)

        if len(offending_lines) > 0:
            self.failure("\n".join(offending_lines))


class GitLint(ComplianceTest):
    """
    Runs gitlint on the commits and finds issues with style and syntax

    """
    name = "Gitlint"
    doc = "See https://docs.zephyrproject.org/latest/contribute/guidelines.html#commit-guidelines for more details"
    path_hint = "<git-top>"

    def run(self):
        # By default gitlint looks for .gitlint configuration only in
        # the current directory
        try:
            subprocess.run('gitlint --commits ' + COMMIT_RANGE,
                           check=True,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT,
                           shell=True, cwd=GIT_TOP)

        except subprocess.CalledProcessError as ex:
            self.failure(ex.output.decode("utf-8"))


class PyLint(ComplianceTest):
    """
    Runs pylint on all .py files, with a limited set of checks enabled. The
    configuration is in the pylintrc file.
    """
    name = "Pylint"
    doc = "See https://www.pylint.org/ for more details"
    path_hint = "<git-top>"

    def run(self):
        # Path to pylint configuration file
        pylintrc = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                                "pylintrc"))

        # Path to additional pylint check scripts
        check_script_dir = os.path.abspath(os.path.join(os.path.dirname(__file__),
                                                        "../pylint/checkers"))

        # List of files added/modified by the commit(s).
        files = get_files(filter="d")

        # Filter out everything but Python files. Keep filenames
        # relative (to GIT_TOP) to stay farther from any command line
        # limit.
        py_files = filter_py(GIT_TOP, files)
        if not py_files:
            return

        python_environment = os.environ.copy()
        if "PYTHONPATH" in python_environment:
            python_environment["PYTHONPATH"] = check_script_dir + ":" + \
                                               python_environment["PYTHONPATH"]
        else:
            python_environment["PYTHONPATH"] = check_script_dir

        pylintcmd = ["pylint", "--output-format=json2", "--rcfile=" + pylintrc,
                     "--load-plugins=argparse-checker"] + py_files
        logger.info(cmd2str(pylintcmd))
        try:
            subprocess.run(pylintcmd,
                           check=True,
                           stdout=subprocess.PIPE,
                           stderr=subprocess.STDOUT,
                           cwd=GIT_TOP,
                           env=python_environment)
        except subprocess.CalledProcessError as ex:
            output = ex.output.decode("utf-8")
            messages = json.loads(output)['messages']
            for m in messages:
                severity = 'unknown'
                if m['messageId'][0] in ('F', 'E'):
                    severity = 'error'
                elif m['messageId'][0] in ('W','C', 'R', 'I'):
                    severity = 'warning'
                self.fmtd_failure(severity, m['messageId'], m['path'],
                                  m['line'], col=str(m['column']), desc=m['message']
                                  + f" ({m['symbol']})")

            if len(messages) == 0:
                # If there are no specific messages add the whole output as a failure
                self.failure(output)


def filter_py(root, fnames):
    # PyLint check helper. Returns all Python script filenames among the
    # filenames in 'fnames', relative to directory 'root'.
    #
    # Uses the python-magic library, so that we can detect Python
    # files that don't end in .py as well. python-magic is a frontend
    # to libmagic, which is also used by 'file'.
    return [fname for fname in fnames
            if (fname.endswith(".py") or
             magic.from_file(os.path.join(root, fname),
                             mime=True) == "text/x-python")]


class Identity(ComplianceTest):
    """
    Checks if Emails of author and signed-off messages are consistent.
    """
    name = "Identity"
    doc = "See https://docs.zephyrproject.org/latest/contribute/guidelines.html#commit-guidelines for more details"
    # git rev-list and git log don't depend on the current (sub)directory
    # unless explicited
    path_hint = "<git-top>"

    def run(self):
        for shaidx in get_shas(COMMIT_RANGE):
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

            error1 = f"{sha}: author email ({author}) needs to match one of " \
                     f"the signed-off-by entries."
            error2 = f"{sha}: author email ({author}) does not follow the " \
                     f"syntax: First Last <email>."
            error3 = f"{sha}: author email ({author}) must be a real email " \
                     f"and cannot end in @users.noreply.github.com"
            failure = None
            if author not in signed:
                failure = error1

            if not parsed_addr or len(parsed_addr[0].split(" ")) < 2:
                if not failure:

                    failure = error2
                else:
                    failure = failure + "\n" + error2
            elif parsed_addr[1].endswith("@users.noreply.github.com"):
                failure = error3

            if failure:
                self.failure(failure)


class BinaryFiles(ComplianceTest):
    """
    Check that the diff contains no binary files.
    """
    name = "BinaryFiles"
    doc = "No binary files allowed."
    path_hint = "<git-top>"

    def run(self):
        BINARY_ALLOW_PATHS = ("doc/", "boards/", "samples/")
        # svg files are always detected as binary, see .gitattributes
        BINARY_ALLOW_EXT = (".jpg", ".jpeg", ".png", ".svg", ".webp")

        for stat in git("diff", "--numstat", "--diff-filter=A",
                        COMMIT_RANGE).splitlines():
            added, deleted, fname = stat.split("\t")
            if added == "-" and deleted == "-":
                if (fname.startswith(BINARY_ALLOW_PATHS) and
                    fname.endswith(BINARY_ALLOW_EXT)):
                    continue
                self.failure(f"Binary file not allowed: {fname}")


class ImageSize(ComplianceTest):
    """
    Check that any added image is limited in size.
    """
    name = "ImageSize"
    doc = "Check the size of image files."
    path_hint = "<git-top>"

    def run(self):
        SIZE_LIMIT = 250 << 10
        BOARD_SIZE_LIMIT = 100 << 10

        for file in get_files(filter="d"):
            full_path = os.path.join(GIT_TOP, file)
            mime_type = magic.from_file(full_path, mime=True)

            if not mime_type.startswith("image/"):
                continue

            size = os.path.getsize(full_path)

            limit = SIZE_LIMIT
            if file.startswith("boards/"):
                limit = BOARD_SIZE_LIMIT

            if size > limit:
                self.failure(f"Image file too large: {file} reduce size to "
                             f"less than {limit >> 10}kB")


class MaintainersFormat(ComplianceTest):
    """
    Check that MAINTAINERS file parses correctly.
    """
    name = "MaintainersFormat"
    doc = "Check that MAINTAINERS file parses correctly."
    path_hint = "<git-top>"

    def run(self):
        MAINTAINERS_FILES = ["MAINTAINERS.yml", "MAINTAINERS.yaml"]

        for file in MAINTAINERS_FILES:
            if not os.path.exists(file):
                continue

            try:
                Maintainers(file)
            except MaintainersError as ex:
                self.failure(f"Error parsing {file}: {ex}")

class ModulesMaintainers(ComplianceTest):
    """
    Check that all modules have a MAINTAINERS entry.
    """
    name = "ModulesMaintainers"
    doc = "Check that all modules have a MAINTAINERS entry."
    path_hint = "<git-top>"

    def run(self):
        MAINTAINERS_FILES = ["MAINTAINERS.yml", "MAINTAINERS.yaml"]

        manifest = Manifest.from_file()

        maintainers_file = None
        for file in MAINTAINERS_FILES:
            if os.path.exists(file):
                maintainers_file = file
                break
        if not maintainers_file:
            return

        maintainers = Maintainers(maintainers_file)

        for project in manifest.get_projects([]):
            if not manifest.is_active(project):
                continue

            if isinstance(project, ManifestProject):
                continue

            area = f"West project: {project.name}"
            if area not in maintainers.areas:
                self.failure(f"Missing {maintainers_file} entry for: \"{area}\"")


class YAMLLint(ComplianceTest):
    """
    YAMLLint
    """
    name = "YAMLLint"
    doc = "Check YAML files with YAMLLint."
    path_hint = "<git-top>"

    def run(self):
        config_file = os.path.join(ZEPHYR_BASE, ".yamllint")

        for file in get_files(filter="d"):
            if Path(file).suffix not in ['.yaml', '.yml']:
                continue

            yaml_config = config.YamlLintConfig(file=config_file)

            if file.startswith(".github/"):
                # Tweak few rules for workflow files.
                yaml_config.rules["line-length"] = False
                yaml_config.rules["truthy"]["allowed-values"].extend(['on', 'off'])
            elif file == ".codecov.yml":
                yaml_config.rules["truthy"]["allowed-values"].extend(['yes', 'no'])

            with open(file, 'r') as fp:
                for p in linter.run(fp, yaml_config):
                    self.fmtd_failure('warning', f'YAMLLint ({p.rule})', file,
                                      p.line, col=p.column, desc=p.desc)


class SphinxLint(ComplianceTest):
    """
    SphinxLint
    """

    name = "SphinxLint"
    doc = "Check Sphinx/reStructuredText files with sphinx-lint."
    path_hint = "<git-top>"

    # Checkers added/removed to sphinx-lint's default set
    DISABLE_CHECKERS = ["horizontal-tab", "missing-space-before-default-role"]
    ENABLE_CHECKERS = ["default-role"]

    def run(self):
        for file in get_files():
            if not file.endswith(".rst"):
                continue

            try:
                # sphinx-lint does not expose a public API so interaction is done via CLI
                subprocess.run(
                    f"sphinx-lint -d {','.join(self.DISABLE_CHECKERS)} -e {','.join(self.ENABLE_CHECKERS)} {file}",
                    check=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    shell=True,
                    cwd=GIT_TOP,
                )

            except subprocess.CalledProcessError as ex:
                for line in ex.output.decode("utf-8").splitlines():
                    match = re.match(r"^(.*):(\d+): (.*)$", line)

                    if match:
                        self.fmtd_failure(
                            "error",
                            "SphinxLint",
                            match.group(1),
                            int(match.group(2)),
                            desc=match.group(3),
                        )


class KeepSorted(ComplianceTest):
    """
    Check for blocks of code or config that should be kept sorted.
    """
    name = "KeepSorted"
    doc = "Check for blocks of code or config that should be kept sorted."
    path_hint = "<git-top>"

    MARKER = "zephyr-keep-sorted"

    def block_check_sorted(self, block_data, regex):
        def _test_indent(txt: str):
            return txt.startswith((" ", "\t"))

        if regex is None:
            block_data = textwrap.dedent(block_data)

        lines = block_data.splitlines()
        last = ''

        for idx, line in enumerate(lines):
            if not line.strip():
                # Ignore blank lines
                continue

            if regex:
                # check for regex
                if not re.match(regex, line):
                    continue
            else:
                if _test_indent(line):
                    continue

                # Fold back indented lines after the current one
                for cont in takewhile(_test_indent, lines[idx + 1:]):
                    line += cont.strip()

            if line < last:
                return idx

            last = line

        return -1

    def check_file(self, file, fp):
        mime_type = magic.from_file(file, mime=True)

        if not mime_type.startswith("text/"):
            return

        block_data = ""
        in_block = False

        start_marker = f"{self.MARKER}-start"
        stop_marker = f"{self.MARKER}-stop"
        regex_marker = r"re\((.+)\)"
        start_line = 0
        regex = None

        for line_num, line in enumerate(fp.readlines(), start=1):
            if start_marker in line:
                if in_block:
                    desc = f"nested {start_marker}"
                    self.fmtd_failure("error", "KeepSorted", file, line_num,
                                     desc=desc)
                in_block = True
                block_data = ""
                start_line = line_num + 1

                # Test for a regex block
                match = re.search(regex_marker, line)
                regex = match.group(1) if match else None
            elif stop_marker in line:
                if not in_block:
                    desc = f"{stop_marker} without {start_marker}"
                    self.fmtd_failure("error", "KeepSorted", file, line_num,
                                     desc=desc)
                in_block = False

                idx = self.block_check_sorted(block_data, regex)
                if idx >= 0:
                    desc = f"sorted block has out-of-order line at {start_line + idx}"
                    self.fmtd_failure("error", "KeepSorted", file, line_num,
                                      desc=desc)
            elif in_block:
                block_data += line

        if in_block:
            self.failure(f"unterminated {start_marker} in {file}")

    def run(self):
        for file in get_files(filter="d"):
            with open(file, "r") as fp:
                self.check_file(file, fp)


class Ruff(ComplianceTest):
    """
    Ruff
    """
    name = "Ruff"
    doc = "Check python files with ruff."
    path_hint = "<git-top>"

    def run(self):
        for file in get_files(filter="d"):
            if not file.endswith(".py"):
                continue

            try:
                subprocess.run(
                    f"ruff check --force-exclude --output-format=json {file}",
                    check=True,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    shell=True,
                    cwd=GIT_TOP,
                )
            except subprocess.CalledProcessError as ex:
                output = ex.output.decode("utf-8")
                messages = json.loads(output)
                for m in messages:
                    self.fmtd_failure(
                        "error",
                        f'Python lint error ({m.get("code")}) see {m.get("url")}',
                        file,
                        line=m.get("location", {}).get("row"),
                        col=m.get("location", {}).get("column"),
                        end_line=m.get("end_location", {}).get("row"),
                        end_col=m.get("end_location", {}).get("column"),
                        desc=m.get("message"),
                    )
            try:
                subprocess.run(
                    f"ruff format --force-exclude --diff {file}",
                    check=True,
                    shell=True,
                    cwd=GIT_TOP,
                )
            except subprocess.CalledProcessError:
                desc = f"Run 'ruff format {file}'"
                self.fmtd_failure("error", "Python format error", file, desc=desc)


class TextEncoding(ComplianceTest):
    """
    Check that any text file is encoded in ascii or utf-8.
    """
    name = "TextEncoding"
    doc = "Check the encoding of text files."
    path_hint = "<git-top>"

    ALLOWED_CHARSETS = ["us-ascii", "utf-8"]

    def run(self):
        m = magic.Magic(mime=True, mime_encoding=True)

        for file in get_files(filter="d"):
            full_path = os.path.join(GIT_TOP, file)
            mime_type = m.from_file(full_path)

            if not mime_type.startswith("text/"):
                continue

            # format is "text/<type>; charset=<charset>"
            if mime_type.rsplit('=')[-1] not in self.ALLOWED_CHARSETS:
                desc = f"Text file with unsupported encoding: {file} has mime type {mime_type}"
                self.fmtd_failure("error", "TextEncoding", file, desc=desc)


def init_logs(cli_arg):
    # Initializes logging

    global logger

    level = os.environ.get('LOG_LEVEL', "WARN")

    console = logging.StreamHandler()
    console.setFormatter(logging.Formatter('%(levelname)-8s: %(message)s'))

    logger = logging.getLogger('')
    logger.addHandler(console)
    logger.setLevel(cli_arg or level)

    logger.info("Log init completed, level=%s",
                 logging.getLevelName(logger.getEffectiveLevel()))


def inheritors(klass):
    subclasses = set()
    work = [klass]
    while work:
        parent = work.pop()
        for child in parent.__subclasses__():
            if child not in subclasses:
                subclasses.add(child)
                work.append(child)
    return subclasses


def annotate(res):
    """
    https://docs.github.com/en/actions/using-workflows/workflow-commands-for-github-actions#about-workflow-commands
    """
    msg = res.message.replace('%', '%25').replace('\n', '%0A').replace('\r', '%0D')
    notice = f'::{res.severity} file={res.file}' + \
             (f',line={res.line}' if res.line else '') + \
             (f',col={res.col}' if res.col else '') + \
             (f',endLine={res.end_line}' if res.end_line else '') + \
             (f',endColumn={res.end_col}' if res.end_col else '') + \
             f',title={res.title}::{msg}'
    print(notice)


def resolve_path_hint(hint):
    if hint == "<zephyr-base>":
        return ZEPHYR_BASE
    elif hint == "<git-top>":
        return GIT_TOP
    else:
        return hint


def parse_args(argv):

    default_range = 'HEAD~1..HEAD'
    parser = argparse.ArgumentParser(
        description="Check for coding style and documentation warnings.", allow_abbrev=False)
    parser.add_argument('-c', '--commits', default=default_range,
                        help=f'''Commit range in the form: a..[b], default is
                        {default_range}''')
    parser.add_argument('-o', '--output', default="compliance.xml",
                        help='''Name of outfile in JUnit format,
                        default is ./compliance.xml''')
    parser.add_argument('-n', '--no-case-output', action="store_true",
                        help="Do not store the individual test case output.")
    parser.add_argument('-l', '--list', action="store_true",
                        help="List all checks and exit")
    parser.add_argument("-v", "--loglevel", choices=['DEBUG', 'INFO', 'WARNING',
                                                     'ERROR', 'CRITICAL'],
                        help="python logging level")
    parser.add_argument('-m', '--module', action="append", default=[],
                        help="Checks to run. All checks by default. (case " \
                        "insensitive)")
    parser.add_argument('-e', '--exclude-module', action="append", default=[],
                        help="Do not run the specified checks (case " \
                        "insensitive)")
    parser.add_argument('-j', '--previous-run', default=None,
                        help='''Pre-load JUnit results in XML format
                        from a previous run and combine with new results.''')
    parser.add_argument('--annotate', action="store_true",
                        help="Print GitHub Actions-compatible annotations.")

    return parser.parse_args(argv)

def _main(args):
    # The "real" main(), which is wrapped to catch exceptions and report them
    # to GitHub. Returns the number of test failures.

    global ZEPHYR_BASE
    ZEPHYR_BASE = os.environ.get('ZEPHYR_BASE')
    if not ZEPHYR_BASE:
        # Let the user run this script as ./scripts/ci/check_compliance.py without
        #  making them set ZEPHYR_BASE.
        ZEPHYR_BASE = str(Path(__file__).resolve().parents[2])

        # Propagate this decision to child processes.
        os.environ['ZEPHYR_BASE'] = ZEPHYR_BASE

    # The absolute path of the top-level git directory. Initialize it here so
    # that issues running Git can be reported to GitHub.
    global GIT_TOP
    GIT_TOP = git("rev-parse", "--show-toplevel")

    # The commit range passed in --commit, e.g. "HEAD~3"
    global COMMIT_RANGE
    COMMIT_RANGE = args.commits

    init_logs(args.loglevel)

    logger.info(f'Running tests on commit range {COMMIT_RANGE}')

    if args.list:
        for testcase in sorted(inheritors(ComplianceTest), key=lambda x: x.name):
            print(testcase.name)
        return 0

    # Load saved test results from an earlier run, if requested
    if args.previous_run:
        if not os.path.exists(args.previous_run):
            # This probably means that an earlier pass had an internal error
            # (the script is currently run multiple times by the ci-pipelines
            # repo). Since that earlier pass might've posted an error to
            # GitHub, avoid generating a GitHub comment here, by avoiding
            # sys.exit() (which gets caught in main()).
            print(f"error: '{args.previous_run}' not found",
                  file=sys.stderr)
            return 1

        logging.info(f"Loading previous results from {args.previous_run}")
        for loaded_suite in JUnitXml.fromfile(args.previous_run):
            suite = loaded_suite
            break
    else:
        suite = TestSuite("Compliance")

    included = list(map(lambda x: x.lower(), args.module))
    excluded = list(map(lambda x: x.lower(), args.exclude_module))

    for testcase in inheritors(ComplianceTest):
        # "Modules" and "testcases" are the same thing. Better flags would have
        # been --tests and --exclude-tests or the like, but it's awkward to
        # change now.

        if included and testcase.name.lower() not in included:
            continue

        if testcase.name.lower() in excluded:
            print("Skipping " + testcase.name)
            continue

        test = testcase()
        try:
            print(f"Running {test.name:16} tests in "
                  f"{resolve_path_hint(test.path_hint)} ...")
            test.run()
        except EndTest:
            pass

        # Annotate if required
        if args.annotate:
            for res in test.fmtd_failures:
                annotate(res)

        suite.add_testcase(test.case)

    if args.output:
        xml = JUnitXml()
        xml.add_testsuite(suite)
        xml.update_statistics()
        xml.write(args.output, pretty=True)

    failed_cases = []
    name2doc = {testcase.name: testcase.doc
                for testcase in inheritors(ComplianceTest)}

    for case in suite:
        if case.result:
            if case.is_skipped:
                logging.warning(f"Skipped {case.name}")
            else:
                failed_cases.append(case)
        else:
            # Some checks like codeowners can produce no .result
            logging.info(f"No JUnit result for {case.name}")

    n_fails = len(failed_cases)

    if n_fails:
        print(f"{n_fails} checks failed")
        for case in failed_cases:
            for res in case.result:
                errmsg = res.text.strip()
                logging.error(f"Test {case.name} failed: \n{errmsg}")
            if args.no_case_output:
                continue
            with open(f"{case.name}.txt", "w") as f:
                docs = name2doc.get(case.name)
                f.write(f"{docs}\n")
                for res in case.result:
                    errmsg = res.text.strip()
                    f.write(f'\n {errmsg}')

    if args.output:
        print(f"\nComplete results in {args.output}")
    return n_fails


def main(argv=None):
    args = parse_args(argv)

    try:
        # pylint: disable=unused-import
        from lxml import etree
    except ImportError:
        print("\nERROR: Python module lxml not installed, unable to proceed")
        print("See https://github.com/weiwei/junitparser/issues/99")
        return 1

    try:
        n_fails = _main(args)
    except BaseException:
        # Catch BaseException instead of Exception to include stuff like
        # SystemExit (raised by sys.exit())
        print(f"Python exception in `{__file__}`:\n\n"
              f"```\n{traceback.format_exc()}\n```")

        raise

    sys.exit(n_fails)


def cmd2str(cmd):
    # Formats the command-line arguments in the iterable 'cmd' into a string,
    # for error messages and the like

    return " ".join(shlex.quote(word) for word in cmd)


def err(msg):
    cmd = sys.argv[0]  # Empty if missing
    if cmd:
        cmd += ": "
    sys.exit(f"{cmd} error: {msg}")


if __name__ == "__main__":
    main(sys.argv[1:])
