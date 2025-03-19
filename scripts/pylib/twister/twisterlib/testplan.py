#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018-2024 Intel Corporation
# Copyright (c) 2024 Arm Limited (or its affiliates). All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0
import collections
import copy
import itertools
import json
import logging
import os
import random
import re
import subprocess
import sys
from argparse import Namespace
from collections import OrderedDict
from itertools import islice
from pathlib import Path

import snippets

try:
    from anytree import Node, RenderTree, find
except ImportError:
    print("Install the anytree module to use the --test-tree option")

import scl
from twisterlib.config_parser import TwisterConfigParser
from twisterlib.error import TwisterRuntimeError
from twisterlib.platform import Platform, generate_platforms
from twisterlib.quarantine import Quarantine
from twisterlib.statuses import TwisterStatus
from twisterlib.testinstance import TestInstance
from twisterlib.testsuite import TestSuite, scan_testsuite_path
from zephyr_module import parse_modules

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
if not ZEPHYR_BASE:
    sys.exit("$ZEPHYR_BASE environment variable undefined")

# This is needed to load edt.pickle files.
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "dts",
                                "python-devicetree", "src"))
from devicetree import edtlib  # pylint: disable=unused-import

sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/"))

class Filters:
    # platform keys
    PLATFORM_KEY = 'platform key filter'
    # filters provided on command line by the user/tester
    CMD_LINE = 'command line filter'
    # filters in the testsuite yaml definition
    TESTSUITE = 'testsuite filter'
    # filters in the testplan yaml definition
    TESTPLAN = 'testplan filter'
    # filters related to platform definition
    PLATFORM = 'Platform related filter'
    # in case a test suite was quarantined.
    QUARANTINE = 'Quarantine filter'
    # in case a test suite is skipped intentionally .
    SKIP = 'Skip filter'
    # in case of incompatibility between selected and allowed toolchains.
    TOOLCHAIN = 'Toolchain filter'
    # in case where an optional module is not available
    MODULE = 'Module filter'
    # in case of missing env. variable required for a platform
    ENVIRONMENT = 'Environment filter'


class TestLevel:
    name = None
    levels = []
    scenarios = []


class TestPlan:
    __test__ = False  # for pytest to skip this class when collects tests
    config_re = re.compile('(CONFIG_[A-Za-z0-9_]+)[=]\"?([^\"]*)\"?$')
    dt_re = re.compile('([A-Za-z0-9_]+)[=]\"?([^\"]*)\"?$')

    suite_schema = scl.yaml_load(
        os.path.join(ZEPHYR_BASE,
                     "scripts", "schemas", "twister", "testsuite-schema.yaml"))
    quarantine_schema = scl.yaml_load(
        os.path.join(ZEPHYR_BASE,
                     "scripts", "schemas", "twister", "quarantine-schema.yaml"))

    tc_schema_path = os.path.join(
        ZEPHYR_BASE,
        "scripts",
        "schemas",
        "twister",
        "test-config-schema.yaml"
    )

    SAMPLE_FILENAME = 'sample.yaml'
    TESTSUITE_FILENAME = 'testcase.yaml'

    def __init__(self, env: Namespace):

        self.options = env.options
        self.env = env

        # Keep track of which test cases we've filtered out and why
        self.testsuites = {}
        self.quarantine = None
        self.platforms = []
        self.platform_names = []
        self.selected_platforms = []
        self.default_platforms = []
        self.load_errors = 0
        self.instances = dict()
        self.instance_fail_count = 0
        self.warnings = 0

        self.scenarios = []

        self.hwm = env.hwm
        # used during creating shorter build paths
        self.link_dir_counter = 0
        self.modules = []

        self.run_individual_testsuite = []
        self.levels = []
        self.test_config =  {}

        self.name = "unnamed"

    def get_level(self, name):
        level = next((lvl for lvl in self.levels if lvl.name == name), None)
        return level

    def parse_configuration(self, config_file):
        if os.path.exists(config_file):
            tc_schema = scl.yaml_load(self.tc_schema_path)
            self.test_config = scl.yaml_load_verify(config_file, tc_schema)
        else:
            raise TwisterRuntimeError(f"File {config_file} not found.")

        levels = self.test_config.get('levels', [])

        # Do first pass on levels to get initial data.
        for level in levels:
            adds = []
            for s in  level.get('adds', []):
                r = re.compile(s)
                adds.extend(list(filter(r.fullmatch, self.scenarios)))

            tl = TestLevel()
            tl.name = level['name']
            tl.scenarios = adds
            tl.levels = level.get('inherits', [])
            self.levels.append(tl)

        # Go over levels again to resolve inheritance.
        for level in levels:
            inherit = level.get('inherits', [])
            _level = self.get_level(level['name'])
            if inherit:
                for inherted_level in inherit:
                    _inherited = self.get_level(inherted_level)
                    assert _inherited, "Unknown inherited level {inherted_level}"
                    _inherited_scenarios = _inherited.scenarios
                    level_scenarios = _level.scenarios if _level else []
                    level_scenarios.extend(_inherited_scenarios)

    def find_subtests(self):
        sub_tests = self.options.sub_test
        if sub_tests:
            for subtest in sub_tests:
                _subtests = self.get_testcase(subtest)
                for _subtest in _subtests:
                    self.run_individual_testsuite.append(_subtest.name)

            if self.run_individual_testsuite:
                logger.info("Running the following tests:")
                for test in self.run_individual_testsuite:
                    print(f" - {test}")
            else:
                raise TwisterRuntimeError("Tests not found")

    def discover(self):
        self.handle_modules()
        if self.options.test:
            self.run_individual_testsuite = self.options.test

        self.add_configurations()
        num = self.add_testsuites(testsuite_filter=self.run_individual_testsuite)
        if num == 0:
            raise TwisterRuntimeError("No testsuites found at the specified location...")
        if self.load_errors:
            raise TwisterRuntimeError(
                f"Found {self.load_errors} errors loading {num} test configurations."
            )

        self.find_subtests()
        # get list of scenarios we have parsed into one list
        for _, ts in self.testsuites.items():
            self.scenarios.append(ts.id)

        self.report_duplicates()
        self.parse_configuration(config_file=self.env.test_config)

        # handle quarantine
        ql = self.options.quarantine_list
        qv = self.options.quarantine_verify
        if qv and not ql:
            logger.error("No quarantine list given to be verified")
            raise TwisterRuntimeError("No quarantine list given to be verified")
        if ql:
            for quarantine_file in ql:
                try:
                    # validate quarantine yaml file against the provided schema
                    scl.yaml_load_verify(quarantine_file, self.quarantine_schema)
                except scl.EmptyYamlFileException:
                    logger.debug(f'Quarantine file {quarantine_file} is empty')
            self.quarantine = Quarantine(ql)

    def load(self):

        if self.options.report_suffix:
            last_run = os.path.join(
                self.options.outdir,
                f"twister_{self.options.report_suffix}.json"
            )
        else:
            last_run = os.path.join(self.options.outdir, "twister.json")

        if self.options.only_failed or self.options.report_summary is not None:
            self.load_from_file(last_run)
            self.selected_platforms = set(p.platform.name for p in self.instances.values())
        elif self.options.load_tests:
            self.load_from_file(self.options.load_tests)
            self.selected_platforms = set(p.platform.name for p in self.instances.values())
        elif self.options.test_only:
            # Get list of connected hardware and filter tests to only be run on connected hardware.
            # If the platform does not exist in the hardware map or was not specified by --platform,
            # just skip it.

            connected_list = []
            excluded_list = []
            for _cp in self.options.platform:
                if _cp in self.platform_names:
                    connected_list.append(self.get_platform(_cp).name)

            if self.options.exclude_platform:
                for _p in self.options.exclude_platform:
                    if _p in self.platform_names:
                        excluded_list.append(self.get_platform(_p).name)
                for excluded in excluded_list:
                    if excluded in connected_list:
                        connected_list.remove(excluded)

            self.load_from_file(last_run, filter_platform=connected_list)
            self.selected_platforms = set(p.platform.name for p in self.instances.values())
        else:
            self.apply_filters()

        if self.options.subset:
            s =  self.options.subset
            try:
                subset, sets = (int(x) for x in s.split("/"))
            except ValueError as err:
                raise TwisterRuntimeError("Bad subset value.") from err

            if subset > sets:
                raise TwisterRuntimeError("subset should not exceed the total number of sets")

            if int(subset) > 0 and int(sets) >= int(subset):
                logger.info(f"Running only a subset: {subset}/{sets}")
            else:
                raise TwisterRuntimeError(
                    f"You have provided a wrong subset value: {self.options.subset}."
                )

            self.generate_subset(subset, int(sets))

    def generate_subset(self, subset, sets):
        # Test instances are sorted depending on the context. For CI runs
        # the execution order is: "plat1-testA, plat1-testB, ...,
        # plat1-testZ, plat2-testA, ...". For hardware tests
        # (device_testing), were multiple physical platforms can run the tests
        # in parallel, it is more efficient to run in the order:
        # "plat1-testA, plat2-testA, ..., plat1-testB, plat2-testB, ..."
        if self.options.device_testing:
            self.instances = OrderedDict(sorted(self.instances.items(),
                                key=lambda x: x[0][x[0].find("/") + 1:]))
        else:
            self.instances = OrderedDict(sorted(self.instances.items()))

        if self.options.shuffle_tests:
            seed_value = int.from_bytes(os.urandom(8), byteorder="big")
            if self.options.shuffle_tests_seed is not None:
                seed_value = self.options.shuffle_tests_seed

            logger.info(f"Shuffle tests with seed: {seed_value}")
            random.seed(seed_value)
            temp_list = list(self.instances.items())
            random.shuffle(temp_list)
            self.instances = OrderedDict(temp_list)

        # Do calculation based on what is actually going to be run and evaluated
        # at runtime, ignore the cases we already know going to be skipped.
        # This fixes an issue where some sets would get majority of skips and
        # basically run nothing beside filtering.
        to_run = {k : v for k,v in self.instances.items() if v.status == TwisterStatus.NONE}
        total = len(to_run)
        per_set = int(total / sets)
        num_extra_sets = total - (per_set * sets)

        # Try and be more fair for rounding error with integer division
        # so the last subset doesn't get overloaded, we add 1 extra to
        # subsets 1..num_extra_sets.
        if subset <= num_extra_sets:
            start = (subset - 1) * (per_set + 1)
            end = start + per_set + 1
        else:
            base = num_extra_sets * (per_set + 1)
            start = ((subset - num_extra_sets - 1) * per_set) + base
            end = start + per_set

        sliced_instances = islice(to_run.items(), start, end)
        skipped = {k : v for k,v in self.instances.items() if v.status == TwisterStatus.SKIP}
        errors = {k : v for k,v in self.instances.items() if v.status == TwisterStatus.ERROR}
        self.instances = OrderedDict(sliced_instances)
        if subset == 1:
            # add all pre-filtered tests that are skipped or got error status
            # to the first set to allow for better distribution among all sets.
            self.instances.update(skipped)
            self.instances.update(errors)


    def handle_modules(self):
        # get all enabled west projects
        modules_meta = parse_modules(ZEPHYR_BASE)
        self.modules = [module.meta.get('name') for module in modules_meta]


    def report(self):
        if self.options.test_tree:
            if not self.options.detailed_test_id:
                logger.info("Test tree is always shown with detailed test-id.")
            self.report_test_tree()
            return 0
        elif self.options.list_tests:
            if not self.options.detailed_test_id:
                logger.info("Test list is always shown with detailed test-id.")
            self.report_test_list()
            return 0
        elif self.options.list_tags:
            self.report_tag_list()
            return 0

        return 1

    def report_duplicates(self):
        dupes = [item for item, count in collections.Counter(self.scenarios).items() if count > 1]
        if dupes:
            msg = "Duplicated test scenarios found:\n"
            for dupe in dupes:
                msg += (f"- {dupe} found in:\n")
                for dc in self.get_testsuite(dupe):
                    msg += (f"  - {dc.yamlfile}\n")
            raise TwisterRuntimeError(msg)
        else:
            logger.debug("No duplicates found.")

    def report_tag_list(self):
        tags = set()
        for _, tc in self.testsuites.items():
            tags = tags.union(tc.tags)

        for t in tags:
            print(f"- {t}")

    def report_test_tree(self):
        tests_list = self.get_tests_list()

        testsuite = Node("Testsuite")
        samples = Node("Samples", parent=testsuite)
        tests = Node("Tests", parent=testsuite)

        for test in sorted(tests_list):
            if test.startswith("sample."):
                sec = test.split(".")
                area = find(
                    samples,
                    lambda node, sname=sec[1]: node.name == sname and node.parent == samples
                )
                if not area:
                    area = Node(sec[1], parent=samples)

                Node(test, parent=area)
            else:
                sec = test.split(".")
                area = find(
                    tests,
                    lambda node, sname=sec[0]: node.name == sname and node.parent == tests
                )
                if not area:
                    area = Node(sec[0], parent=tests)

                if area and len(sec) > 2:
                    subarea = find(
                        area, lambda node, sname=sec[1], sparent=area: node.name == sname
                        and node.parent == sparent
                    )
                    if not subarea:
                        subarea = Node(sec[1], parent=area)
                    Node(test, parent=subarea)

        for pre, _, node in RenderTree(testsuite):
            print(f"{pre}{node.name}")

    def report_test_list(self):
        tests_list = self.get_tests_list()

        cnt = 0
        for test in sorted(tests_list):
            cnt = cnt + 1
            print(f" - {test}")
        print(f"{cnt} total.")


    # Debug Functions
    @staticmethod
    def info(what):
        sys.stdout.write(what + "\n")
        sys.stdout.flush()

    def add_configurations(self):
        # Create a list of board roots as defined by the build system in general
        # Note, internally in twister a board root includes the `boards` folder
        # but in Zephyr build system, the board root is without the `boards` in folder path.
        board_roots = [Path(os.path.dirname(root)) for root in self.env.board_roots]
        soc_roots = self.env.soc_roots
        arch_roots = self.env.arch_roots

        platform_config = self.test_config.get('platforms', {})

        for platform in generate_platforms(board_roots, soc_roots, arch_roots):
            if not platform.twister:
                continue
            self.platforms.append(platform)

            if not platform_config.get('override_default_platforms', False):
                if platform.default:
                    self.default_platforms.append(platform.name)
                    #logger.debug(f"adding {platform.name} to default platforms")
                continue
            for pp in platform_config.get('default_platforms', []):
                if pp in platform.aliases:
                    logger.debug(f"adding {platform.name} to default platforms (override  mode)")
                    self.default_platforms.append(platform.name)

        self.platform_names = [a for p in self.platforms for a in p.aliases]

    def get_all_tests(self):
        testcases = []
        for _, ts in self.testsuites.items():
            for case in ts.testcases:
                testcases.append(case.name)

        return testcases

    def get_tests_list(self):
        testcases = []
        if tag_filter := self.options.tag:
            for _, ts in self.testsuites.items():
                if ts.tags.intersection(tag_filter):
                    for case in ts.testcases:
                        testcases.append(case.detailed_name)
        else:
            for _, ts in self.testsuites.items():
                for case in ts.testcases:
                    testcases.append(case.detailed_name)

        if exclude_tag := self.options.exclude_tag:
            for _, ts in self.testsuites.items():
                if ts.tags.intersection(exclude_tag):
                    for case in ts.testcases:
                        if case.detailed_name in testcases:
                            testcases.remove(case.detailed_name)
        return testcases

    def add_testsuites(self, testsuite_filter=None):
        if testsuite_filter is None:
            testsuite_filter = []
        for root in self.env.test_roots:
            root = os.path.abspath(root)

            logger.debug(f"Reading testsuite configuration files under {root}...")

            for dirpath, _, filenames in os.walk(root, topdown=True):
                if self.SAMPLE_FILENAME in filenames:
                    filename = self.SAMPLE_FILENAME
                elif self.TESTSUITE_FILENAME in filenames:
                    filename = self.TESTSUITE_FILENAME
                else:
                    continue

                logger.debug("Found possible testsuite in " + dirpath)

                suite_yaml_path = os.path.join(dirpath, filename)
                suite_path = os.path.dirname(suite_yaml_path)

                for alt_config_root in self.env.alt_config_root:
                    alt_config = os.path.join(os.path.abspath(alt_config_root),
                                              os.path.relpath(suite_path, root),
                                              filename)
                    if os.path.exists(alt_config):
                        logger.info(
                            f"Using alternative configuration from {os.path.normpath(alt_config)}"
                        )
                        suite_yaml_path = alt_config
                        break

                try:
                    parsed_data = TwisterConfigParser(suite_yaml_path, self.suite_schema)
                    parsed_data.load()
                    subcases = None
                    ztest_suite_names = None

                    for name in parsed_data.scenarios:
                        suite_dict = parsed_data.get_scenario(name)
                        suite = TestSuite(
                            root,
                            suite_path,
                            name,
                            data=suite_dict,
                            detailed_test_id=self.options.detailed_test_id
                        )

                        # convert to fully qualified names
                        suite.integration_platforms = self.verify_platforms_existence(
                                suite.integration_platforms,
                                f"integration_platforms in {suite.name}")
                        suite.platform_exclude = self.verify_platforms_existence(
                                suite.platform_exclude,
                                f"platform_exclude in {suite.name}")
                        suite.platform_allow =  self.verify_platforms_existence(
                                suite.platform_allow,
                                f"platform_allow in {suite.name}")

                        if suite.harness in ['ztest', 'test']:
                            if subcases is None:
                                # scan it only once per testsuite
                                subcases, ztest_suite_names = scan_testsuite_path(suite_path)
                            suite.add_subcases(suite_dict, subcases, ztest_suite_names)
                        else:
                            suite.add_subcases(suite_dict)

                        if testsuite_filter:
                            scenario = os.path.basename(suite.name)
                            if (
                                suite.name
                                and (suite.name in testsuite_filter or scenario in testsuite_filter)
                            ):
                                self.testsuites[suite.name] = suite
                        elif suite.name in self.testsuites:
                            msg = (
                                f"test suite '{suite.name}' in '{suite.yamlfile}' is already added"
                            )
                            if suite.yamlfile == self.testsuites[suite.name].yamlfile:
                                logger.debug(f"Skip - {msg}")
                            else:
                                msg = (
                                    f"Duplicate {msg} from '{self.testsuites[suite.name].yamlfile}'"
                                )
                                raise TwisterRuntimeError(msg)
                        else:
                            self.testsuites[suite.name] = suite

                except Exception as e:
                    logger.error(f"{suite_path}: can't load (skipping): {e!r}")
                    self.load_errors += 1
        return len(self.testsuites)

    def __str__(self):
        return self.name

    def get_platform(self, name):
        selected_platform = None
        for platform in self.platforms:
            if name in platform.aliases:
                selected_platform = platform
                break
        return selected_platform

    def handle_quarantined_tests(self, instance: TestInstance, plat: Platform):
        if self.quarantine:
            simulator = plat.simulator_by_name(self.options)
            matched_quarantine = self.quarantine.get_matched_quarantine(
                instance.testsuite.id,
                plat.name,
                plat.arch,
                simulator.name if simulator is not None else 'na'
            )
            if matched_quarantine and not self.options.quarantine_verify:
                instance.add_filter("Quarantine: " + matched_quarantine, Filters.QUARANTINE)
                return
            if not matched_quarantine and self.options.quarantine_verify:
                instance.add_filter("Not under quarantine", Filters.QUARANTINE)

    def load_from_file(self, file, filter_platform=None):
        if filter_platform is None:
            filter_platform = []
        try:
            with open(file) as json_test_plan:
                jtp = json.load(json_test_plan)
                instance_list = []
                for ts in jtp.get("testsuites", []):
                    logger.debug(f"loading {ts['name']}...")
                    testsuite = ts["name"]
                    toolchain = ts["toolchain"]

                    platform = self.get_platform(ts["platform"])
                    if filter_platform and platform.name not in filter_platform:
                        continue
                    instance = TestInstance(
                        self.testsuites[testsuite], platform, toolchain, self.env.outdir
                    )
                    if ts.get("run_id"):
                        instance.run_id = ts.get("run_id")

                    instance.run = instance.check_runnable(
                        self.options,
                        self.hwm
                    )

                    if self.options.test_only and not instance.run:
                        continue

                    instance.metrics['handler_time'] = ts.get('execution_time', 0)
                    instance.metrics['used_ram'] = ts.get("used_ram", 0)
                    instance.metrics['used_rom']  = ts.get("used_rom",0)
                    instance.metrics['available_ram'] = ts.get('available_ram', 0)
                    instance.metrics['available_rom'] = ts.get('available_rom', 0)

                    status = TwisterStatus(ts.get('status'))
                    reason = ts.get("reason", "Unknown")
                    if status in [TwisterStatus.ERROR, TwisterStatus.FAIL]:
                        if self.options.report_summary is not None:
                            instance.status = status
                            instance.reason = reason
                            self.instance_fail_count += 1
                        else:
                            instance.status = TwisterStatus.NONE
                            instance.reason = None
                            instance.retries += 1
                    # test marked as built only can run when --test-only is used.
                    # Reset status to capture new results.
                    elif status == TwisterStatus.NOTRUN and instance.run and self.options.test_only:
                        instance.status = TwisterStatus.NONE
                        instance.reason = None
                    else:
                        instance.status = status
                        instance.reason = reason

                    self.handle_quarantined_tests(instance, platform)

                    for tc in ts.get('testcases', []):
                        identifier = tc['identifier']
                        tc_status = TwisterStatus(tc.get('status'))
                        tc_reason = None
                        # we set reason only if status is valid, it might have been
                        # reset above...
                        if instance.status != TwisterStatus.NONE:
                            tc_reason = tc.get('reason')
                        if tc_status != TwisterStatus.NONE:
                            case = instance.set_case_status_by_name(
                                identifier,
                                tc_status,
                                tc_reason
                            )
                            case.duration = tc.get('execution_time', 0)
                            if tc.get('log'):
                                case.output = tc.get('log')

                    instance.create_overlay(platform,
                                            self.options.enable_asan,
                                            self.options.enable_ubsan,
                                            self.options.enable_coverage,
                                            self.options.coverage_platform
                                            )
                    instance_list.append(instance)
                self.add_instances(instance_list)
        except FileNotFoundError as e:
            logger.error(f"{e}")
            return 1

    def check_platform(self, platform, platform_list):
        return any(p in platform.aliases for p in platform_list)

    def apply_filters(self, **kwargs):

        platform_filter = self.options.platform
        vendor_filter = self.options.vendor
        exclude_platform = self.options.exclude_platform
        testsuite_filter = self.run_individual_testsuite
        arch_filter = self.options.arch
        tag_filter = self.options.tag
        exclude_tag = self.options.exclude_tag
        all_filter = self.options.all
        runnable = (self.options.device_testing or self.options.filter == 'runnable')
        force_toolchain = self.options.force_toolchain
        force_platform = self.options.force_platform
        slow_only = self.options.enable_slow_only
        ignore_platform_key = self.options.ignore_platform_key
        emu_filter = self.options.emulation_only

        logger.debug("platform filter: " + str(platform_filter))
        logger.debug("  vendor filter: " + str(vendor_filter))
        logger.debug("    arch_filter: " + str(arch_filter))
        logger.debug("     tag_filter: " + str(tag_filter))
        logger.debug("    exclude_tag: " + str(exclude_tag))

        default_platforms = False
        vendor_platforms = False
        emulation_platforms = False

        if all_filter:
            logger.info("Selecting all possible platforms per testsuite scenario")
            # When --all used, any --platform arguments ignored
            platform_filter = []
        elif not platform_filter and not emu_filter and not vendor_filter:
            logger.info("Selecting default platforms per testsuite scenario")
            default_platforms = True
        elif emu_filter:
            logger.info("Selecting emulation platforms per testsuite scenraio")
            emulation_platforms = True
        elif vendor_filter:
            vendor_platforms = True

        _platforms = []
        if platform_filter:
            logger.debug(f"Checking platform filter: {platform_filter}")
            # find in aliases and rename
            platform_filter = self.verify_platforms_existence(platform_filter, "platform_filter")
            platforms = list(filter(lambda p: p.name in platform_filter, self.platforms))
        elif emu_filter:
            platforms = list(
                filter(lambda p: bool(p.simulator_by_name(self.options.sim_name)), self.platforms)
            )
        elif vendor_filter:
            platforms = list(filter(lambda p: p.vendor in vendor_filter, self.platforms))
            logger.info(f"Selecting platforms by vendors: {','.join(vendor_filter)}")
        elif arch_filter:
            platforms = list(filter(lambda p: p.arch in arch_filter, self.platforms))
        elif default_platforms:
            _platforms = list(filter(lambda p: p.name in self.default_platforms, self.platforms))
            platforms = []
            # default platforms that can't be run are dropped from the list of
            # the default platforms list. Default platforms should always be
            # runnable.
            for p in _platforms:
                sim = p.simulator_by_name(self.options.sim_name)
                if (not sim) or sim.is_runnable():
                    platforms.append(p)
        else:
            platforms = self.platforms

        platform_config = self.test_config.get('platforms', {})
        logger.info("Building initial testsuite list...")

        keyed_tests = {}

        for _, ts in self.testsuites.items():
            if (
                ts.build_on_all
                and not platform_filter
                and platform_config.get('increased_platform_scope', True)
            ):
                platform_scope = self.platforms
            elif ts.integration_platforms:
                integration_platforms = list(
                    filter(lambda item: item.name in ts.integration_platforms, self.platforms)
                )
                if self.options.integration:
                    platform_scope = integration_platforms
                else:
                    # if not in integration mode, still add integration platforms to the list
                    if not platform_filter:
                        platform_scope = platforms + integration_platforms
                    else:
                        platform_scope = platforms
            else:
                platform_scope = platforms

            integration = self.options.integration and ts.integration_platforms

            # If there isn't any overlap between the platform_allow list and the platform_scope
            # we set the scope to the platform_allow list
            if (
                ts.platform_allow
                and not platform_filter
                and not integration
                and platform_config.get('increased_platform_scope', True)
            ):
                a = set(platform_scope)
                b = set(filter(lambda item: item.name in ts.platform_allow, self.platforms))
                c = a.intersection(b)
                if not c:
                    platform_scope = list(
                        filter(lambda item: item.name in ts.platform_allow, self.platforms)
                    )
            # list of instances per testsuite, aka configurations.
            instance_list = []
            for itoolchain, plat in itertools.product(
                ts.integration_toolchains or [None], platform_scope
            ):
                if itoolchain:
                    toolchain = itoolchain
                elif plat.arch in ['posix', 'unit']:
                    # workaround until toolchain variant in zephyr is overhauled and improved.
                    if self.env.toolchain in ['llvm']:
                        toolchain = 'llvm'
                    else:
                        toolchain = 'host'
                else:
                    toolchain = "zephyr" if not self.env.toolchain else self.env.toolchain

                instance = TestInstance(ts, plat, toolchain, self.env.outdir)
                instance.run = instance.check_runnable(
                    self.options,
                    self.hwm
                )

                if not force_platform and self.check_platform(plat,exclude_platform):
                    instance.add_filter("Platform is excluded on command line.", Filters.CMD_LINE)

                if (plat.arch == "unit") != (ts.type == "unit"):
                    # Discard silently
                    continue

                if ts.modules and self.modules and not set(ts.modules).issubset(set(self.modules)):
                    instance.add_filter(
                        f"one or more required modules not available: {','.join(ts.modules)}",
                        Filters.MODULE
                    )

                if self.options.level:
                    tl = self.get_level(self.options.level)
                    if tl is None:
                        instance.add_filter(
                            f"Unknown test level '{self.options.level}'",
                            Filters.TESTPLAN
                        )
                    else:
                        planned_scenarios = tl.scenarios
                        if (
                            ts.id not in planned_scenarios
                            and not set(ts.levels).intersection(set(tl.levels))
                        ):
                            instance.add_filter("Not part of requested test plan", Filters.TESTPLAN)

                if runnable and not instance.run:
                    instance.add_filter("Not runnable on device", Filters.CMD_LINE)

                if (
                    self.options.integration
                    and ts.integration_platforms
                    and plat.name not in ts.integration_platforms
                ):
                    instance.add_filter("Not part of integration platforms", Filters.TESTSUITE)

                if ts.skip:
                    instance.add_filter("Skip filter", Filters.SKIP)

                if tag_filter and not ts.tags.intersection(tag_filter):
                    instance.add_filter("Command line testsuite tag filter", Filters.CMD_LINE)

                if slow_only and not ts.slow:
                    instance.add_filter("Not a slow test", Filters.CMD_LINE)

                if exclude_tag and ts.tags.intersection(exclude_tag):
                    instance.add_filter("Command line testsuite exclude filter", Filters.CMD_LINE)

                if testsuite_filter:
                    normalized_f = [os.path.basename(_ts) for _ts in testsuite_filter]
                    if ts.id not in normalized_f:
                        instance.add_filter("Testsuite name filter", Filters.CMD_LINE)

                if arch_filter and plat.arch not in arch_filter:
                    instance.add_filter("Command line testsuite arch filter", Filters.CMD_LINE)

                if not force_platform:

                    if ts.arch_allow and plat.arch not in ts.arch_allow:
                        instance.add_filter("Not in testsuite arch allow list", Filters.TESTSUITE)

                    if ts.arch_exclude and plat.arch in ts.arch_exclude:
                        instance.add_filter("In testsuite arch exclude", Filters.TESTSUITE)

                    if ts.vendor_allow and plat.vendor not in ts.vendor_allow:
                        instance.add_filter(
                            "Not in testsuite vendor allow list",
                            Filters.TESTSUITE
                        )

                    if ts.vendor_exclude and plat.vendor in ts.vendor_exclude:
                        instance.add_filter("In testsuite vendor exclude", Filters.TESTSUITE)

                    if ts.platform_exclude and plat.name in ts.platform_exclude:
                        instance.add_filter("In testsuite platform exclude", Filters.TESTSUITE)

                if ts.toolchain_exclude and toolchain in ts.toolchain_exclude:
                    instance.add_filter("In testsuite toolchain exclude", Filters.TOOLCHAIN)

                if platform_filter and plat.name not in platform_filter:
                    instance.add_filter("Command line platform filter", Filters.CMD_LINE)

                if ts.platform_allow \
                        and plat.name not in ts.platform_allow \
                        and not (platform_filter and force_platform):
                    instance.add_filter("Not in testsuite platform allow list", Filters.TESTSUITE)

                if ts.platform_type and plat.type not in ts.platform_type:
                    instance.add_filter("Not in testsuite platform type list", Filters.TESTSUITE)

                if ts.toolchain_allow and toolchain not in ts.toolchain_allow:
                    instance.add_filter("Not in testsuite toolchain allow list", Filters.TOOLCHAIN)

                if not plat.env_satisfied:
                    instance.add_filter(
                        "Environment ({}) not satisfied".format(", ".join(plat.env)),
                        Filters.ENVIRONMENT
                    )
                if plat.type == 'native' and sys.platform != 'linux':
                    instance.add_filter("Native platform requires Linux", Filters.ENVIRONMENT)

                if not force_toolchain \
                        and toolchain and (toolchain not in plat.supported_toolchains):
                    instance.add_filter(
                        f"Not supported by the toolchain: {toolchain}",
                        Filters.PLATFORM
                    )

                if plat.ram < ts.min_ram:
                    instance.add_filter("Not enough RAM", Filters.PLATFORM)

                if ts.harness:
                    sim = plat.simulator_by_name(self.options.sim_name)
                    if ts.harness == 'robot' and not (sim and sim.name == 'renode'):
                        instance.add_filter(
                            "No robot support for the selected platform",
                            Filters.SKIP
                        )

                if ts.depends_on:
                    dep_intersection = ts.depends_on.intersection(set(plat.supported))
                    if dep_intersection != set(ts.depends_on):
                        instance.add_filter(
                            f"No hardware support for {set(ts.depends_on)-dep_intersection}",
                            Filters.PLATFORM
                        )

                if plat.flash < ts.min_flash:
                    instance.add_filter("Not enough FLASH", Filters.PLATFORM)

                if set(plat.ignore_tags) & ts.tags:
                    instance.add_filter(
                        "Excluded tags per platform (exclude_tags)",
                        Filters.PLATFORM
                    )

                if plat.only_tags and not set(plat.only_tags) & ts.tags:
                    instance.add_filter("Excluded tags per platform (only_tags)", Filters.PLATFORM)

                if ts.required_snippets:
                    missing_snippet = False
                    snippet_args = {"snippets": ts.required_snippets}
                    found_snippets = snippets.find_snippets_in_roots(
                        snippet_args,
                        [*self.env.snippet_roots, Path(ts.source_dir)]
                    )

                    # Search and check that all required snippet files are found
                    for this_snippet in snippet_args['snippets']:
                        if this_snippet not in found_snippets:
                            logger.error(
                                f"Can't find snippet '{this_snippet}' for test '{ts.name}'"
                            )
                            instance.status = TwisterStatus.ERROR
                            instance.reason = f"Snippet {this_snippet} not found"
                            missing_snippet = True
                            break

                    if not missing_snippet:
                        # Look for required snippets and check that they are applicable for these
                        # platforms/boards
                        for this_snippet in snippet_args['snippets']:
                            matched_snippet_board = False

                            # If the "appends" key is present with at least one entry then this
                            # snippet applies to all boards and further platform-specific checks
                            # are not required
                            if found_snippets[this_snippet].appends:
                                continue

                            for this_board in found_snippets[this_snippet].board2appends:
                                if this_board.startswith('/'):
                                    match = re.search(this_board[1:-1], plat.name)
                                    if match is not None:
                                        matched_snippet_board = True
                                        break
                                elif this_board == plat.name:
                                    matched_snippet_board = True
                                    break

                            if matched_snippet_board is False:
                                instance.add_filter("Snippet not supported", Filters.PLATFORM)
                                break

                # handle quarantined tests
                self.handle_quarantined_tests(instance, plat)

                # platform_key is a list of unique platform attributes that form a unique key
                # a test will match against to determine if it should be scheduled to run.
                # A key containing a field name that the platform does not have
                # will filter the platform.
                #
                # A simple example is keying on arch and simulation
                # to run a test once per unique (arch, simulation) platform.
                if (
                    not ignore_platform_key
                    and hasattr(ts, 'platform_key')
                    and len(ts.platform_key) > 0
                ):
                    key_fields = sorted(set(ts.platform_key))
                    keys = [getattr(plat, key_field, None) for key_field in key_fields]
                    for key in keys:
                        if key is None or key == 'na':
                            instance.add_filter(
                                "Excluded platform missing key fields"
                                f" demanded by test {key_fields}",
                                Filters.PLATFORM
                            )
                            break
                    else:
                        test_keys = copy.deepcopy(keys)
                        test_keys.append(ts.name)
                        test_keys = tuple(test_keys)
                        keyed_test = keyed_tests.get(test_keys)
                        if keyed_test is not None:
                            plat_key = {
                                key_field: getattr(
                                    keyed_test['plat'],
                                    key_field
                                ) for key_field in key_fields
                            }
                            instance.add_filter(
                                f"Already covered for key {key}"
                                f" by platform {keyed_test['plat'].name} having key {plat_key}",
                                Filters.PLATFORM_KEY
                            )
                        else:
                            # do not add a platform to keyed tests if previously
                            # filtered

                            if not instance.filters:
                                keyed_tests[test_keys] = {'plat': plat, 'ts': ts}

                # if nothing stopped us until now, it means this configuration
                # needs to be added.
                instance_list.append(instance)

            # no configurations, so jump to next testsuite
            if not instance_list:
                continue

            # if twister was launched with no platform options at all, we
            # take all default platforms
            if default_platforms and not ts.build_on_all and not integration:
                if ts.platform_allow:
                    _default_p = set(self.default_platforms)
                    _platform_allow = set(ts.platform_allow)
                    _intersection = _default_p.intersection(_platform_allow)
                    if _intersection:
                        aa = list(
                            filter(
                                lambda _scenario: _scenario.platform.name in _intersection,
                                instance_list
                            )
                        )
                        self.add_instances(aa)
                    else:
                        self.add_instances(instance_list)
                else:
                    # add integration platforms to the list of default
                    # platforms, even if we are not in integration mode
                    _platforms = self.default_platforms + ts.integration_platforms
                    instances = list(
                        filter(lambda ts: ts.platform.name in _platforms, instance_list)
                    )
                    self.add_instances(instances)
            elif integration:
                instances = list(
                    filter(
                        lambda item:  item.platform.name in ts.integration_platforms,
                        instance_list
                    )
                )
                self.add_instances(instances)

            elif emulation_platforms:
                self.add_instances(instance_list)
                for instance in list(
                    filter(
                        lambda inst: not inst.platform.simulator_by_name(self.options.sim_name),
                        instance_list
                    )
                ):
                    instance.add_filter("Not an emulated platform", Filters.CMD_LINE)
            elif vendor_platforms:
                self.add_instances(instance_list)
                for instance in list(
                    filter(
                        lambda inst: inst.platform.vendor not in vendor_filter,
                        instance_list
                    )
                ):
                    instance.add_filter("Not a selected vendor platform", Filters.CMD_LINE)
            else:
                self.add_instances(instance_list)

        for _, case in self.instances.items():
            # Do not create files for filtered instances
            if case.status == TwisterStatus.FILTER:
                continue
            # set run_id for each unfiltered instance
            case.setup_run_id()
            case.create_overlay(case.platform,
                                self.options.enable_asan,
                                self.options.enable_ubsan,
                                self.options.enable_coverage,
                                self.options.coverage_platform)

        self.selected_platforms = set(p.platform.name for p in self.instances.values())

        filtered_instances = list(
            filter(lambda item:  item.status == TwisterStatus.FILTER, self.instances.values())
        )
        for filtered_instance in filtered_instances:
            change_skip_to_error_if_integration(self.options, filtered_instance)

            filtered_instance.add_missing_case_status(filtered_instance.status)

    def add_instances(self, instance_list):
        for instance in instance_list:
            self.instances[instance.name] = instance


    def get_testsuite(self, identifier):
        results = []
        for _, ts in self.testsuites.items():
            if ts.id == identifier:
                results.append(ts)
        return results

    def get_testcase(self, identifier):
        results = []
        for _, ts in self.testsuites.items():
            for case in ts.testcases:
                if case.name == identifier:
                    results.append(ts)
        return results

    def verify_platforms_existence(self, platform_names_to_verify, log_info=""):
        """
        Verify if platform name (passed by --platform option, or in yaml file
        as platform_allow or integration_platforms options) is correct. If not -
        log and raise error.
        """
        _platforms = []
        for platform in platform_names_to_verify:
            if platform in self.platform_names:
                p = self.get_platform(platform)
                if p:
                    _platforms.append(p.name)
            else:
                logger.error(f"{log_info} - unrecognized platform - {platform}")
                sys.exit(2)
        return _platforms

    def create_build_dir_links(self):
        """
        Iterate through all no-skipped instances in suite and create links
        for each one build directories. Those links will be passed in the next
        steps to the CMake command.
        """

        links_dir_name = "twister_links"  # folder for all links
        links_dir_path = os.path.join(self.env.outdir, links_dir_name)
        if not os.path.exists(links_dir_path):
            os.mkdir(links_dir_path)

        for instance in self.instances.values():
            if instance.status != TwisterStatus.SKIP:
                self._create_build_dir_link(links_dir_path, instance)

    def _create_build_dir_link(self, links_dir_path, instance):
        """
        Create build directory with original "long" path. Next take shorter
        path and link them with original path - create link. At the end
        replace build_dir to created link. This link will be passed to CMake
        command. This action helps to limit path length which can be
        significant during building by CMake on Windows OS.
        """

        os.makedirs(instance.build_dir, exist_ok=True)

        link_name = f"test_{self.link_dir_counter}"
        link_path = os.path.join(links_dir_path, link_name)

        if os.name == "nt":  # if OS is Windows
            command = ["mklink", "/J", f"{link_path}", os.path.normpath(instance.build_dir)]
            subprocess.call(command, shell=True)
        else:  # for Linux and MAC OS
            os.symlink(instance.build_dir, link_path)

        # Here original build directory is replaced with symbolic link. It will
        # be passed to CMake command
        instance.build_dir = link_path

        self.link_dir_counter += 1


def change_skip_to_error_if_integration(options, instance):
    ''' All skips on integration_platforms are treated as errors.'''
    if instance.platform.name in instance.testsuite.integration_platforms:
        # Do not treat this as error if filter type is among ignore_filters
        filters = {t['type'] for t in instance.filters}
        ignore_filters ={Filters.CMD_LINE, Filters.SKIP, Filters.PLATFORM_KEY,
                         Filters.TOOLCHAIN, Filters.MODULE, Filters.TESTPLAN,
                         Filters.QUARANTINE, Filters.ENVIRONMENT}
        if filters.intersection(ignore_filters):
            return
        instance.status = TwisterStatus.ERROR
        instance.reason += " but is one of the integration platforms"
        logger.debug(
            f"Changing status of {instance.name} to ERROR because it is an integration platform"
        )
