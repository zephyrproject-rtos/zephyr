# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018-2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import os
from pathlib import Path
import re
import logging
import contextlib
import mmap
import glob
from typing import List
from twisterlib.mixins import DisablePyTestCollectionMixin
from twisterlib.environment import canonical_zephyr_base
from twisterlib.error import TwisterException, TwisterRuntimeError

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)

class ScanPathResult:
    """Result of the scan_tesuite_path function call.

    Attributes:
        matches                          A list of test cases
        warnings                         A string containing one or more
                                         warnings to display
        has_registered_test_suites       Whether or not the path contained any
                                         calls to the ztest_register_test_suite
                                         macro.
        has_run_registered_test_suites   Whether or not the path contained at
                                         least one call to
                                         ztest_run_registered_test_suites.
        has_test_main                    Whether or not the path contains a
                                         definition of test_main(void)
        ztest_suite_names                Names of found ztest suites
    """
    def __init__(self,
                 matches: List[str] = None,
                 warnings: str = None,
                 has_registered_test_suites: bool = False,
                 has_run_registered_test_suites: bool = False,
                 has_test_main: bool = False,
                 ztest_suite_names: List[str] = []):
        self.matches = matches
        self.warnings = warnings
        self.has_registered_test_suites = has_registered_test_suites
        self.has_run_registered_test_suites = has_run_registered_test_suites
        self.has_test_main = has_test_main
        self.ztest_suite_names = ztest_suite_names

    def __eq__(self, other):
        if not isinstance(other, ScanPathResult):
            return False
        return (sorted(self.matches) == sorted(other.matches) and
                self.warnings == other.warnings and
                (self.has_registered_test_suites ==
                 other.has_registered_test_suites) and
                (self.has_run_registered_test_suites ==
                 other.has_run_registered_test_suites) and
                self.has_test_main == other.has_test_main and
                (sorted(self.ztest_suite_names) ==
                 sorted(other.ztest_suite_names)))

def scan_file(inf_name):
    regular_suite_regex = re.compile(
        # do not match until end-of-line, otherwise we won't allow
        # stc_regex below to catch the ones that are declared in the same
        # line--as we only search starting the end of this match
        br"^\s*ztest_test_suite\(\s*(?P<suite_name>[a-zA-Z0-9_]+)\s*,",
        re.MULTILINE)
    registered_suite_regex = re.compile(
        br"^\s*ztest_register_test_suite"
        br"\(\s*(?P<suite_name>[a-zA-Z0-9_]+)\s*,",
        re.MULTILINE)
    new_suite_regex = re.compile(
        br"^\s*ZTEST_SUITE\(\s*(?P<suite_name>[a-zA-Z0-9_]+)\s*,",
        re.MULTILINE)
    testcase_regex = re.compile(
        br"^\s*(?:ZTEST|ZTEST_F|ZTEST_USER|ZTEST_USER_F)\(\s*(?P<suite_name>[a-zA-Z0-9_]+)\s*,"
        br"\s*(?P<testcase_name>[a-zA-Z0-9_]+)\s*",
        re.MULTILINE)
    # Checks if the file contains a definition of "void test_main(void)"
    # Since ztest provides a plain test_main implementation it is OK to:
    # 1. register test suites and not call the run function if and only if
    #    the test doesn't have a custom test_main.
    # 2. register test suites and a custom test_main definition if and only if
    #    the test also calls ztest_run_registered_test_suites.
    test_main_regex = re.compile(
        br"^\s*void\s+test_main\(void\)",
        re.MULTILINE)
    registered_suite_run_regex = re.compile(
        br"^\s*ztest_run_registered_test_suites\("
        br"(\*+|&)?(?P<state_identifier>[a-zA-Z0-9_]+)\)",
        re.MULTILINE)

    warnings = None
    has_registered_test_suites = False
    has_run_registered_test_suites = False
    has_test_main = False

    with open(inf_name) as inf:
        if os.name == 'nt':
            mmap_args = {'fileno': inf.fileno(), 'length': 0, 'access': mmap.ACCESS_READ}
        else:
            mmap_args = {'fileno': inf.fileno(), 'length': 0, 'flags': mmap.MAP_PRIVATE, 'prot': mmap.PROT_READ,
                            'offset': 0}

        with contextlib.closing(mmap.mmap(**mmap_args)) as main_c:
            regular_suite_regex_matches = \
                [m for m in regular_suite_regex.finditer(main_c)]
            registered_suite_regex_matches = \
                [m for m in registered_suite_regex.finditer(main_c)]
            new_suite_testcase_regex_matches = \
                [m for m in testcase_regex.finditer(main_c)]
            new_suite_regex_matches = \
                [m for m in new_suite_regex.finditer(main_c)]

            if registered_suite_regex_matches:
                has_registered_test_suites = True
            if registered_suite_run_regex.search(main_c):
                has_run_registered_test_suites = True
            if test_main_regex.search(main_c):
                has_test_main = True

            if regular_suite_regex_matches:
                ztest_suite_names = \
                    _extract_ztest_suite_names(regular_suite_regex_matches)
                testcase_names, warnings = \
                    _find_regular_ztest_testcases(main_c, regular_suite_regex_matches, has_registered_test_suites)
            elif registered_suite_regex_matches:
                ztest_suite_names = \
                    _extract_ztest_suite_names(registered_suite_regex_matches)
                testcase_names, warnings = \
                    _find_regular_ztest_testcases(main_c, registered_suite_regex_matches, has_registered_test_suites)
            elif new_suite_regex_matches or new_suite_testcase_regex_matches:
                ztest_suite_names = \
                    _extract_ztest_suite_names(new_suite_regex_matches)
                testcase_names, warnings = \
                    _find_new_ztest_testcases(main_c)
            else:
                # can't find ztest_test_suite, maybe a client, because
                # it includes ztest.h
                ztest_suite_names = []
                testcase_names, warnings = None, None

            return ScanPathResult(
                matches=testcase_names,
                warnings=warnings,
                has_registered_test_suites=has_registered_test_suites,
                has_run_registered_test_suites=has_run_registered_test_suites,
                has_test_main=has_test_main,
                ztest_suite_names=ztest_suite_names)

def _extract_ztest_suite_names(suite_regex_matches):
    ztest_suite_names = \
        [m.group("suite_name") for m in suite_regex_matches]
    ztest_suite_names = \
        [name.decode("UTF-8") for name in ztest_suite_names]
    return ztest_suite_names

def _find_regular_ztest_testcases(search_area, suite_regex_matches, is_registered_test_suite):
    """
    Find regular ztest testcases like "ztest_unit_test" or similar. Return
    testcases' names and eventually found warnings.
    """
    testcase_regex = re.compile(
        br"""^\s*  # empty space at the beginning is ok
        # catch the case where it is declared in the same sentence, e.g:
        #
        # ztest_test_suite(mutex_complex, ztest_user_unit_test(TESTNAME));
        # ztest_register_test_suite(n, p, ztest_user_unit_test(TESTNAME),
        (?:ztest_
            (?:test_suite\(|register_test_suite\([a-zA-Z0-9_]+\s*,\s*)
            [a-zA-Z0-9_]+\s*,\s*
        )?
        # Catch ztest[_user]_unit_test-[_setup_teardown](TESTNAME)
        ztest_(?:1cpu_)?(?:user_)?unit_test(?:_setup_teardown)?
        # Consume the argument that becomes the extra testcase
        \(\s*(?P<testcase_name>[a-zA-Z0-9_]+)
        # _setup_teardown() variant has two extra arguments that we ignore
        (?:\s*,\s*[a-zA-Z0-9_]+\s*,\s*[a-zA-Z0-9_]+)?
        \s*\)""",
        # We don't check how it finishes; we don't care
        re.MULTILINE | re.VERBOSE)
    achtung_regex = re.compile(
        br"(#ifdef|#endif)",
        re.MULTILINE)

    search_start, search_end = \
        _get_search_area_boundary(search_area, suite_regex_matches, is_registered_test_suite)
    limited_search_area = search_area[search_start:search_end]
    testcase_names, warnings = \
        _find_ztest_testcases(limited_search_area, testcase_regex)

    achtung_matches = re.findall(achtung_regex, limited_search_area)
    if achtung_matches and warnings is None:
        achtung = ", ".join(sorted({match.decode() for match in achtung_matches},reverse = True))
        warnings = f"found invalid {achtung} in ztest_test_suite()"

    return testcase_names, warnings


def _get_search_area_boundary(search_area, suite_regex_matches, is_registered_test_suite):
    """
    Get search area boundary based on "ztest_test_suite(...)",
    "ztest_register_test_suite(...)" or "ztest_run_test_suite(...)"
    functions occurrence.
    """
    suite_run_regex = re.compile(
        br"^\s*ztest_run_test_suite\((?P<suite_name>[a-zA-Z0-9_]+)\)",
        re.MULTILINE)

    search_start = suite_regex_matches[0].end()

    suite_run_match = suite_run_regex.search(search_area)
    if suite_run_match:
        search_end = suite_run_match.start()
    elif not suite_run_match and not is_registered_test_suite:
        raise ValueError("can't find ztest_run_test_suite")
    else:
        search_end = re.compile(br"\);", re.MULTILINE) \
            .search(search_area, search_start) \
            .end()

    return search_start, search_end

def _find_new_ztest_testcases(search_area):
    """
    Find regular ztest testcases like "ZTEST", "ZTEST_F" etc. Return
    testcases' names and eventually found warnings.
    """
    testcase_regex = re.compile(
        br"^\s*(?:ZTEST|ZTEST_F|ZTEST_USER|ZTEST_USER_F)\(\s*(?P<suite_name>[a-zA-Z0-9_]+)\s*,"
        br"\s*(?P<testcase_name>[a-zA-Z0-9_]+)\s*",
        re.MULTILINE)

    return _find_ztest_testcases(search_area, testcase_regex)

def _find_ztest_testcases(search_area, testcase_regex):
    """
    Parse search area and try to find testcases defined in testcase_regex
    argument. Return testcase names and eventually found warnings.
    """
    testcase_regex_matches = \
        [m for m in testcase_regex.finditer(search_area)]
    testcase_names = \
        [m.group("testcase_name") for m in testcase_regex_matches]
    testcase_names = [name.decode("UTF-8") for name in testcase_names]
    warnings = None
    for testcase_name in testcase_names:
        if not testcase_name.startswith("test_"):
            warnings = "Found a test that does not start with test_"
    testcase_names = \
        [tc_name.replace("test_", "", 1) for tc_name in testcase_names]

    return testcase_names, warnings

def find_c_files_in(path: str, extensions: list = ['c', 'cpp', 'cxx', 'cc']) -> list:
    """
    Find C or C++ sources in the directory specified by "path"
    """
    if not os.path.isdir(path):
        return []

    # back up previous CWD
    oldpwd = os.getcwd()
    os.chdir(path)

    filenames = []
    for ext in extensions:
        # glob.glob('**/*.c') does not pick up the base directory
        filenames += [os.path.join(path, x) for x in glob.glob(f'*.{ext}')]
        # glob matches in subdirectories too
        filenames += [os.path.join(path, x) for x in glob.glob(f'**/*.{ext}')]

    # restore previous CWD
    os.chdir(oldpwd)

    return filenames

def scan_testsuite_path(testsuite_path):
    subcases = []
    has_registered_test_suites = False
    has_run_registered_test_suites = False
    has_test_main = False
    ztest_suite_names = []

    src_dir_path = _find_src_dir_path(testsuite_path)
    for filename in find_c_files_in(src_dir_path):
        if os.stat(filename).st_size == 0:
            continue
        try:
            result: ScanPathResult = scan_file(filename)
            if result.warnings:
                logger.error("%s: %s" % (filename, result.warnings))
                raise TwisterRuntimeError(
                    "%s: %s" % (filename, result.warnings))
            if result.matches:
                subcases += result.matches
            if result.has_registered_test_suites:
                has_registered_test_suites = True
            if result.has_run_registered_test_suites:
                has_run_registered_test_suites = True
            if result.has_test_main:
                has_test_main = True
            if result.ztest_suite_names:
                ztest_suite_names += result.ztest_suite_names

        except ValueError as e:
            logger.error("%s: error parsing source file: %s" % (filename, e))

    src_dir_pathlib_path = Path(src_dir_path)
    for filename in find_c_files_in(testsuite_path):
        # If we have already scanned those files in the src_dir step, skip them.
        filename_path = Path(filename)
        if src_dir_pathlib_path in filename_path.parents:
            continue

        try:
            result: ScanPathResult = scan_file(filename)
            if result.warnings:
                logger.error("%s: %s" % (filename, result.warnings))
            if result.matches:
                subcases += result.matches
            if result.ztest_suite_names:
                ztest_suite_names += result.ztest_suite_names
        except ValueError as e:
            logger.error("%s: can't find: %s" % (filename, e))

    if (has_registered_test_suites and has_test_main and
            not has_run_registered_test_suites):
        warning = \
            "Found call to 'ztest_register_test_suite()' but no "\
            "call to 'ztest_run_registered_test_suites()'"
        logger.error(warning)
        raise TwisterRuntimeError(warning)

    return subcases, ztest_suite_names

def _find_src_dir_path(test_dir_path):
    """
    Try to find src directory with test source code. Sometimes due to the
    optimization reasons it is placed in upper directory.
    """
    src_dir_name = "src"
    src_dir_path = os.path.join(test_dir_path, src_dir_name)
    if os.path.isdir(src_dir_path):
        return src_dir_path
    src_dir_path = os.path.join(test_dir_path, "..", src_dir_name)
    if os.path.isdir(src_dir_path):
        return src_dir_path
    return ""

class TestCase(DisablePyTestCollectionMixin):

    def __init__(self, name=None, testsuite=None):
        self.duration = 0
        self.name = name
        self.status = None
        self.reason = None
        self.testsuite = testsuite
        self.output = ""
        self.freeform = False

    def __lt__(self, other):
        return self.name < other.name

    def __repr__(self):
        return "<TestCase %s with %s>" % (self.name, self.status)

    def __str__(self):
        return self.name

class TestSuite(DisablePyTestCollectionMixin):
    """Class representing a test application
    """

    def __init__(self, suite_root, suite_path, name, data=None, detailed_test_id=True):
        """TestSuite constructor.

        This gets called by TestPlan as it finds and reads test yaml files.
        Multiple TestSuite instances may be generated from a single testcase.yaml,
        each one corresponds to an entry within that file.

        We need to have a unique name for every single test case. Since
        a testcase.yaml can define multiple tests, the canonical name for
        the test case is <workdir>/<name>.

        @param testsuite_root os.path.abspath() of one of the --testsuite-root
        @param suite_path path to testsuite
        @param name Name of this test case, corresponding to the entry name
            in the test case configuration file. For many test cases that just
            define one test, can be anything and is usually "test". This is
            really only used to distinguish between different cases when
            the testcase.yaml defines multiple tests
        """

        workdir = os.path.relpath(suite_path, suite_root)

        assert self.check_suite_name(name, suite_root, workdir)
        self.detailed_test_id = detailed_test_id
        self.name = self.get_unique(suite_root, workdir, name) if self.detailed_test_id else name
        self.id = name

        self.source_dir = suite_path
        self.source_dir_rel = os.path.relpath(os.path.realpath(suite_path), start=canonical_zephyr_base)
        self.yamlfile = suite_path
        self.testcases = []
        self.integration_platforms = []

        self.ztest_suite_names = []

        if data:
            self.load(data)


    def load(self, data):
        for k, v in data.items():
            if k != "testcases":
                setattr(self, k, v)

        if self.harness == 'console' and not self.harness_config:
            raise Exception('Harness config error: console harness defined without a configuration.')

    def add_subcases(self, data, parsed_subcases, suite_names):
        testcases = data.get("testcases", [])
        if testcases:
            for tc in testcases:
                self.add_testcase(name=f"{self.id}.{tc}")
        else:
            # only add each testcase once
            for sub in set(parsed_subcases):
                name = "{}.{}".format(self.id, sub)
                self.add_testcase(name)

            if not parsed_subcases:
                self.add_testcase(self.id, freeform=True)

        self.ztest_suite_names = suite_names

    def add_testcase(self, name, freeform=False):
        tc = TestCase(name=name, testsuite=self)
        tc.freeform = freeform
        self.testcases.append(tc)

    @staticmethod
    def get_unique(testsuite_root, workdir, name):

        canonical_testsuite_root = os.path.realpath(testsuite_root)
        if Path(canonical_zephyr_base) in Path(canonical_testsuite_root).parents:
            # This is in ZEPHYR_BASE, so include path in name for uniqueness
            # FIXME: We should not depend on path of test for unique names.
            relative_ts_root = os.path.relpath(canonical_testsuite_root,
                                               start=canonical_zephyr_base)
        else:
            relative_ts_root = ""

        # workdir can be "."
        unique = os.path.normpath(os.path.join(relative_ts_root, workdir, name)).replace(os.sep, '/')
        return unique

    @staticmethod
    def check_suite_name(name, testsuite_root, workdir):
        check = name.split(".")
        if len(check) < 2:
            raise TwisterException(f"""bad test name '{name}' in {testsuite_root}/{workdir}. \
Tests should reference the category and subsystem with a dot as a separator.
                    """
                    )
        return True
