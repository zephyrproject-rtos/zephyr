# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018-2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import os
import sys
from pathlib import Path
from twister.mixins import DisablePyTestCollectionMixin
from twister.enviornment import canonical_zephyr_base
from twister.error import TwisterException

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

    def __init__(self, testsuite_root, workdir, name):
        """TestSuite constructor.

        This gets called by TestPlan as it finds and reads test yaml files.
        Multiple TestSuite instances may be generated from a single testcase.yaml,
        each one corresponds to an entry within that file.

        We need to have a unique name for every single test case. Since
        a testcase.yaml can define multiple tests, the canonical name for
        the test case is <workdir>/<name>.

        @param testsuite_root os.path.abspath() of one of the --testsuite-root
        @param workdir Sub-directory of testsuite_root where the
            .yaml test configuration file was found
        @param name Name of this test case, corresponding to the entry name
            in the test case configuration file. For many test cases that just
            define one test, can be anything and is usually "test". This is
            really only used to distinguish between different cases when
            the testcase.yaml defines multiple tests
        """


        self.source_dir = ""
        self.yamlfile = ""
        self.testcases = []
        self.name = self.get_unique(testsuite_root, workdir, name)
        self.id = name

        self.type = None
        self.tags = set()
        self.extra_args = None
        self.extra_configs = None
        self.arch_allow = None
        self.arch_exclude = None
        self.skip = False
        self.platform_exclude = None
        self.platform_allow = None
        self.platform_type = []
        self.toolchain_exclude = None
        self.toolchain_allow = None
        self.ts_filter = None
        self.timeout = 60
        self.harness = ""
        self.harness_config = {}
        self.build_only = True
        self.build_on_all = False
        self.slow = False
        self.min_ram = -1
        self.depends_on = None
        self.min_flash = -1
        self.extra_sections = None
        self.integration_platforms = []
        self.ztest_suite_names = []


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
            relative_tc_root = os.path.relpath(canonical_testsuite_root,
                                               start=canonical_zephyr_base)
        else:
            relative_tc_root = ""

        # workdir can be "."
        unique = os.path.normpath(os.path.join(relative_tc_root, workdir, name))
        check = name.split(".")
        if len(check) < 2:
            raise TwisterException(f"""bad test name '{name}' in {testsuite_root}/{workdir}. \
Tests should reference the category and subsystem with a dot as a separator.
                    """
                    )
        return unique

