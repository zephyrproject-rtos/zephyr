#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
import os
import sys
import re
import subprocess
import glob
import json
import collections
from collections import OrderedDict
from itertools import islice
import logging
import copy
import shutil

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)

try:
    from anytree import RenderTree, Node, find
except ImportError:
    print("Install the anytree module to use the --test-tree option")

from twisterlib.testsuite import TestSuite, scan_testsuite_path
from twisterlib.error import TwisterRuntimeError
from twisterlib.platform import Platform
from twisterlib.config_parser import TwisterConfigParser
from twisterlib.testinstance import TestInstance
from twisterlib.quarantine import Quarantine


from zephyr_module import parse_modules

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
if not ZEPHYR_BASE:
    sys.exit("$ZEPHYR_BASE environment variable undefined")

# This is needed to load edt.pickle files.
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "dts",
                                "python-devicetree", "src"))
from devicetree import edtlib  # pylint: disable=unused-import

sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/"))

import scl
class Filters:
    # filters provided on command line by the user/tester
    CMD_LINE = 'command line filter'
    # filters in the testsuite yaml definition
    TESTSUITE = 'testsuite filter'
    # filters realted to platform definition
    PLATFORM = 'Platform related filter'
    # in case a test suite was quarantined.
    QUARENTINE = 'Quarantine filter'
    # in case a test suite is skipped intentionally .
    SKIP = 'Skip filter'


class TestPlan:
    config_re = re.compile('(CONFIG_[A-Za-z0-9_]+)[=]\"?([^\"]*)\"?$')
    dt_re = re.compile('([A-Za-z0-9_]+)[=]\"?([^\"]*)\"?$')

    suite_schema = scl.yaml_load(
        os.path.join(ZEPHYR_BASE,
                     "scripts", "schemas", "twister", "testsuite-schema.yaml"))
    quarantine_schema = scl.yaml_load(
        os.path.join(ZEPHYR_BASE,
                     "scripts", "schemas", "twister", "quarantine-schema.yaml"))

    SAMPLE_FILENAME = 'sample.yaml'
    TESTSUITE_FILENAME = 'testcase.yaml'

    def __init__(self, env=None):

        self.options = env.options
        self.env = env

        # Keep track of which test cases we've filtered out and why
        self.testsuites = {}
        self.quarantine = None
        self.platforms = []
        self.platform_names = []
        self.selected_platforms = []
        self.filtered_platforms = []
        self.default_platforms = []
        self.load_errors = 0
        self.instances = dict()
        self.warnings = 0

        self.hwm = env.hwm
        # used during creating shorter build paths
        self.link_dir_counter = 0
        self.modules = []

        self.run_individual_testsuite = []

    def find_subtests(self):
        sub_tests = self.options.sub_test
        if sub_tests:
            for subtest in sub_tests:
                _subtests = self.get_testsuite(subtest)
                for _subtest in _subtests:
                    self.run_individual_testsuite.append(_subtest.name)

            if self.run_individual_testsuite:
                logger.info("Running the following tests:")
                for test in self.run_individual_testsuite:
                    print(" - {}".format(test))
            else:
                raise TwisterRuntimeError("Tests not found")

    def discover(self):
        self.handle_modules()
        if self.options.test:
            self.run_individual_testsuite = self.options.test

        num = self.add_testsuites(testsuite_filter=self.run_individual_testsuite)
        if num == 0:
            raise TwisterRuntimeError("No test cases found at the specified location...")

        self.find_subtests()
        self.add_configurations()

        if self.load_errors:
            raise TwisterRuntimeError("Errors while loading configurations")

        # handle quarantine
        ql = self.options.quarantine_list
        qv = self.options.quarantine_verify
        if qv and not ql:
            logger.error("No quarantine list given to be verified")
            raise TwisterRuntimeError("No quarantine list given to be verified")
        if ql:
            for quarantine_file in ql:
                # validate quarantine yaml file against the provided schema
                scl.yaml_load_verify(quarantine_file, self.quarantine_schema)
            self.quarantine = Quarantine(ql)

    def load(self):

        if self.options.report_suffix:
            last_run = os.path.join(self.options.outdir, "twister_{}.json".format(self.options.report_suffix))
        else:
            last_run = os.path.join(self.options.outdir, "twister.json")

        if self.options.only_failed:
            self.load_from_file(last_run)
            self.selected_platforms = set(p.platform.name for p in self.instances.values())
        elif self.options.load_tests:
            self.load_from_file(self.options.load_tests)
            self.selected_platforms = set(p.platform.name for p in self.instances.values())
        elif self.options.test_only:
            # Get list of connected hardware and filter tests to only be run on connected hardware
            # in cases where no platform was specified when running the tests.
            # If the platform does not exist in the hardware map, just skip it.
            connected_list = []
            if not self.options.platform:
                for connected in self.hwm.duts:
                    if connected['connected']:
                        connected_list.append(connected['platform'])

            self.load_from_file(last_run, filter_platform=connected_list)
            self.selected_platforms = set(p.platform.name for p in self.instances.values())
        else:
            self.apply_filters()

        if self.options.subset:
            s =  self.options.subset
            try:
                subset, sets = (int(x) for x in s.split("/"))
            except ValueError:
                raise TwisterRuntimeError("Bad subset value.")

            if subset > sets:
                raise TwisterRuntimeError("subset should not exceed the total number of sets")

            if int(subset) > 0 and int(sets) >= int(subset):
                logger.info("Running only a subset: %s/%s" % (subset, sets))
            else:
                raise TwisterRuntimeError(f"You have provided a wrong subset value: {self.options.subset}.")

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

        # Do calculation based on what is actually going to be run and evaluated
        # at runtime, ignore the cases we already know going to be skipped.
        # This fixes an issue where some sets would get majority of skips and
        # basically run nothing beside filtering.
        to_run = {k : v for k,v in self.instances.items() if v.status is None}
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
        skipped = {k : v for k,v in self.instances.items() if v.status == 'skipped'}
        errors = {k : v for k,v in self.instances.items() if v.status == 'error'}
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
        if self.options.list_test_duplicates:
            self.report_duplicates()
            return 0
        elif self.options.test_tree:
            self.report_test_tree()
            return 0
        elif self.options.list_tests:
            self.report_test_list()
            return 0
        elif self.options.list_tags:
            self.report_tag_list()
            return 0

        return 1

    def report_duplicates(self):
        all_identifiers = []
        for _, ts in self.testsuites.items():
            all_identifiers.append(ts.id)
        dupes = [item for item, count in collections.Counter(all_identifiers).items() if count > 1]
        if dupes:
            print("Tests with duplicate identifiers:")
            for dupe in dupes:
                print("- {}".format(dupe))
                for dc in self.get_testsuite(dupe):
                    print("  - {}".format(dc.name))
        else:
            print("No duplicates found.")

    def report_tag_list(self):
        tags = set()
        for _, tc in self.testsuites.items():
            tags = tags.union(tc.tags)

        for t in tags:
            print("- {}".format(t))

    def report_test_tree(self):
        all_tests = self.get_all_tests()

        testsuite = Node("Testsuite")
        samples = Node("Samples", parent=testsuite)
        tests = Node("Tests", parent=testsuite)

        for test in sorted(all_tests):
            if test.startswith("sample."):
                sec = test.split(".")
                area = find(samples, lambda node: node.name == sec[1] and node.parent == samples)
                if not area:
                    area = Node(sec[1], parent=samples)

                Node(test, parent=area)
            else:
                sec = test.split(".")
                area = find(tests, lambda node: node.name == sec[0] and node.parent == tests)
                if not area:
                    area = Node(sec[0], parent=tests)

                if area and len(sec) > 2:
                    subarea = find(area, lambda node: node.name == sec[1] and node.parent == area)
                    if not subarea:
                        subarea = Node(sec[1], parent=area)
                    Node(test, parent=subarea)

        for pre, _, node in RenderTree(testsuite):
            print("%s%s" % (pre, node.name))

    def report_test_list(self):
        cnt = 0
        all_tests = self.get_all_tests()

        for test in sorted(all_tests):
            cnt = cnt + 1
            print(" - {}".format(test))

        print("{} total.".format(cnt))


    def report_excluded_tests(self):
        all_tests = self.get_all_tests()
        to_be_run = set()
        for _, p in self.instances.items():
            to_be_run.update(p.testsuite.cases)

        if all_tests - to_be_run:
            print("Tests that never build or run:")
            for not_run in all_tests - to_be_run:
                print("- {}".format(not_run))

    def report_platform_tests(self, platforms=[]):
        if len(platforms) > 1:
            raise TwisterRuntimeError("When exporting tests, only one platform "
                                      "should be specified.")

        for p in platforms:
            inst = self.get_platform_instances(p)
            count = 0
            for i in inst.values():
                for c in i.testsuite.cases:
                    print(f"- {c}")
                    count += 1
            print(f"Tests found: {count}")

    def get_platform_instances(self, platform):
        filtered_dict = {k:v for k,v in self.instances.items() if k.startswith(platform + os.sep)}
        return filtered_dict

    def config(self):
        logger.info("coverage platform: {}".format(self.coverage_platform))

    # Debug Functions
    @staticmethod
    def info(what):
        sys.stdout.write(what + "\n")
        sys.stdout.flush()


    def add_configurations(self):
        for board_root in self.env.board_roots:
            board_root = os.path.abspath(board_root)
            logger.debug("Reading platform configuration files under %s..." %
                         board_root)

            for file in glob.glob(os.path.join(board_root, "*", "*", "*.yaml")):
                try:
                    platform = Platform()
                    platform.load(file)
                    if platform.name in [p.name for p in self.platforms]:
                        logger.error(f"Duplicate platform {platform.name} in {file}")
                        raise Exception(f"Duplicate platform identifier {platform.name} found")
                    if platform.twister:
                        self.platforms.append(platform)
                        if platform.default:
                            self.default_platforms.append(platform.name)
                        # support board@revision
                        # if there is already an existed <board>_<revision>.yaml, then use it to
                        # load platform directly, otherwise, iterate the directory to
                        # get all valid board revision based on each <board>_<revision>.conf.
                        if not "@" in platform.name:
                            tmp_dir = os.listdir(os.path.dirname(file))
                            for item in tmp_dir:
                                # Need to make sure the revision matches
                                # the permitted patterns as described in
                                # cmake/modules/extensions.cmake.
                                revision_patterns = ["[A-Z]",
                                                     "[0-9]+",
                                                     "(0|[1-9][0-9]*)(_[0-9]+)*(_[0-9]+)*"]

                                for pattern in revision_patterns:
                                    result = re.match(f"{platform.name}_(?P<revision>{pattern})\\.conf", item)
                                    if result:
                                        revision = result.group("revision")
                                        yaml_file = f"{platform.name}_{revision}.yaml"
                                        if yaml_file not in tmp_dir:
                                            platform_revision = copy.deepcopy(platform)
                                            revision = revision.replace("_", ".")
                                            platform_revision.name = f"{platform.name}@{revision}"
                                            platform_revision.default = False
                                            self.platforms.append(platform_revision)

                                        break


                except RuntimeError as e:
                    logger.error("E: %s: can't load: %s" % (file, e))
                    self.load_errors += 1

        self.platform_names = [p.name for p in self.platforms]

    def get_all_tests(self):
        testcases = []
        for _, ts in self.testsuites.items():
            for case in ts.testcases:
                testcases.append(case.name)

        return testcases

    def add_testsuites(self, testsuite_filter=[]):
        for root in self.env.test_roots:
            root = os.path.abspath(root)

            logger.debug("Reading test case configuration files under %s..." % root)

            for dirpath, _, filenames in os.walk(root, topdown=True):
                if self.SAMPLE_FILENAME in filenames:
                    filename = self.SAMPLE_FILENAME
                elif self.TESTSUITE_FILENAME in filenames:
                    filename = self.TESTSUITE_FILENAME
                else:
                    continue

                logger.debug("Found possible testsuite in " + dirpath)

                suite_yaml_path = os.path.join(dirpath, filename)

                try:
                    parsed_data = TwisterConfigParser(suite_yaml_path, self.suite_schema)
                    parsed_data.load()

                    suite_path = os.path.dirname(suite_yaml_path)

                    subcases, ztest_suite_names = scan_testsuite_path(suite_path)

                    for name in parsed_data.scenarios.keys():
                        suite_dict = parsed_data.get_scenario(name)
                        suite = TestSuite(root, suite_path, name, data=suite_dict)
                        suite.add_subcases(suite_dict, subcases, ztest_suite_names)
                        if testsuite_filter:
                            if suite.name and suite.name in testsuite_filter:
                                self.testsuites[suite.name] = suite
                        else:
                            self.testsuites[suite.name] = suite

                except Exception as e:
                    logger.error("%s: can't load (skipping): %s" % (suite_path, e))
                    self.load_errors += 1
        return len(self.testsuites)

    def __str__(self):
        return self.name

    def get_platform(self, name):
        selected_platform = None
        for platform in self.platforms:
            if platform.name == name:
                selected_platform = platform
                break
        return selected_platform

    def load_from_file(self, file, filter_platform=[]):
        with open(file, "r") as json_test_plan:
            jtp = json.load(json_test_plan)
            instance_list = []
            for ts in jtp.get("testsuites", []):
                logger.debug(f"loading {ts['name']}...")
                testsuite = ts["name"]

                platform = self.get_platform(ts["platform"])
                if filter_platform and platform.name not in filter_platform:
                    continue
                instance = TestInstance(self.testsuites[testsuite], platform, self.env.outdir)
                if ts.get("run_id"):
                    instance.run_id = ts.get("run_id")

                if self.options.device_testing:
                    tfilter = 'runnable'
                else:
                    tfilter = 'buildable'
                instance.run = instance.check_runnable(
                    self.options.enable_slow,
                    tfilter,
                    self.options.fixture
                )

                instance.metrics['handler_time'] = ts.get('execution_time', 0)
                instance.metrics['used_ram'] = ts.get("used_ram", 0)
                instance.metrics['used_rom']  = ts.get("used_rom",0)
                instance.metrics['available_ram'] = ts.get('available_ram', 0)
                instance.metrics['available_rom'] = ts.get('available_rom', 0)

                status = ts.get('status', None)
                reason = ts.get("reason", "Unknown")
                if status in ["error", "failed"]:
                    instance.status = None
                    instance.reason = None
                    instance.retries += 1
                # test marked as passed (built only) but can run when
                # --test-only is used. Reset status to capture new results.
                elif status == 'passed' and instance.run and self.options.test_only:
                    instance.status = None
                    instance.reason = None
                else:
                    instance.status = status
                    instance.reason = reason

                for tc in ts.get('testcases', []):
                    identifier = tc['identifier']
                    tc_status = tc.get('status', None)
                    tc_reason = None
                    # we set reason only if status is valid, it might have been
                    # reset above...
                    if instance.status:
                        tc_reason = tc.get('reason')
                    if tc_status:
                        case = instance.set_case_status_by_name(identifier, tc_status, tc_reason)
                        case.duration = tc.get('execution_time', 0)
                        if tc.get('log'):
                            case.output = tc.get('log')


                instance.create_overlay(platform, self.options.enable_asan, self.options.enable_ubsan, self.options.enable_coverage, self.options.coverage_platform)
                instance_list.append(instance)
            self.add_instances(instance_list)

    def apply_filters(self, **kwargs):

        toolchain = self.env.toolchain
        platform_filter = self.options.platform
        exclude_platform = self.options.exclude_platform
        testsuite_filter = self.run_individual_testsuite
        arch_filter = self.options.arch
        tag_filter = self.options.tag
        exclude_tag = self.options.exclude_tag
        all_filter = self.options.all
        runnable = (self.options.device_testing or self.options.filter == 'runnable')
        force_toolchain = self.options.force_toolchain
        force_platform = self.options.force_platform
        ignore_platform_key = self.options.ignore_platform_key
        emu_filter = self.options.emulation_only

        logger.debug("platform filter: " + str(platform_filter))
        logger.debug("    arch_filter: " + str(arch_filter))
        logger.debug("     tag_filter: " + str(tag_filter))
        logger.debug("    exclude_tag: " + str(exclude_tag))

        default_platforms = False
        emulation_platforms = False


        if all_filter:
            logger.info("Selecting all possible platforms per test case")
            # When --all used, any --platform arguments ignored
            platform_filter = []
        elif not platform_filter and not emu_filter:
            logger.info("Selecting default platforms per test case")
            default_platforms = True
        elif emu_filter:
            logger.info("Selecting emulation platforms per test case")
            emulation_platforms = True

        if platform_filter:
            self.verify_platforms_existence(platform_filter, f"platform_filter")
            platforms = list(filter(lambda p: p.name in platform_filter, self.platforms))
        elif emu_filter:
            platforms = list(filter(lambda p: p.simulation != 'na', self.platforms))
        elif arch_filter:
            platforms = list(filter(lambda p: p.arch in arch_filter, self.platforms))
        elif default_platforms:
            _platforms = list(filter(lambda p: p.default, self.platforms))
            platforms = []
            # default platforms that can't be run are dropped from the list of
            # the default platforms list. Default platforms should always be
            # runnable.
            for p in _platforms:
                if p.simulation and p.simulation_exec:
                    if shutil.which(p.simulation_exec):
                        platforms.append(p)
                else:
                    platforms.append(p)
        else:
            platforms = self.platforms

        logger.info("Building initial testsuite list...")

        keyed_tests = {}

        for ts_name, ts in self.testsuites.items():

            if ts.build_on_all and not platform_filter:
                platform_scope = self.platforms
            elif ts.integration_platforms and self.options.integration:
                self.verify_platforms_existence(
                    ts.integration_platforms, f"{ts_name} - integration_platforms")
                platform_scope = list(filter(lambda item: item.name in ts.integration_platforms, \
                                         self.platforms))
            else:
                platform_scope = platforms

            integration = self.options.integration and ts.integration_platforms

            # If there isn't any overlap between the platform_allow list and the platform_scope
            # we set the scope to the platform_allow list
            if ts.platform_allow and not platform_filter and not integration:
                self.verify_platforms_existence(
                    ts.platform_allow, f"{ts_name} - platform_allow")
                a = set(platform_scope)
                b = set(filter(lambda item: item.name in ts.platform_allow, self.platforms))
                c = a.intersection(b)
                if not c:
                    platform_scope = list(filter(lambda item: item.name in ts.platform_allow, \
                                             self.platforms))


            # list of instances per testsuite, aka configurations.
            instance_list = []
            for plat in platform_scope:
                instance = TestInstance(ts, plat, self.env.outdir)
                if runnable:
                    tfilter = 'runnable'
                else:
                    tfilter = 'buildable'

                instance.run = instance.check_runnable(
                    self.options.enable_slow,
                    tfilter,
                    self.options.fixture
                )
                if runnable and self.hwm.duts:
                    for h in self.hwm.duts:
                        if h.platform == plat.name:
                            if ts.harness_config.get('fixture') in h.fixtures:
                                instance.run = True

                if not force_platform and plat.name in exclude_platform:
                    instance.add_filter("Platform is excluded on command line.", Filters.CMD_LINE)

                if (plat.arch == "unit") != (ts.type == "unit"):
                    # Discard silently
                    continue

                if ts.modules and self.modules:
                    if not set(ts.modules).issubset(set(self.modules)):
                        instance.add_filter(f"one or more required modules not available: {','.join(ts.modules)}", Filters.TESTSUITE)

                if runnable and not instance.run:
                    instance.add_filter("Not runnable on device", Filters.PLATFORM)

                if self.options.integration and ts.integration_platforms and plat.name not in ts.integration_platforms:
                    instance.add_filter("Not part of integration platforms", Filters.TESTSUITE)

                if ts.skip:
                    instance.add_filter("Skip filter", Filters.SKIP)

                if tag_filter and not ts.tags.intersection(tag_filter):
                    instance.add_filter("Command line testsuite tag filter", Filters.CMD_LINE)

                if exclude_tag and ts.tags.intersection(exclude_tag):
                    instance.add_filter("Command line testsuite exclude filter", Filters.CMD_LINE)

                if testsuite_filter and ts_name not in testsuite_filter:
                    instance.add_filter("TestSuite name filter", Filters.CMD_LINE)

                if arch_filter and plat.arch not in arch_filter:
                    instance.add_filter("Command line testsuite arch filter", Filters.CMD_LINE)

                if not force_platform:

                    if ts.arch_allow and plat.arch not in ts.arch_allow:
                        instance.add_filter("Not in test case arch allow list", Filters.TESTSUITE)

                    if ts.arch_exclude and plat.arch in ts.arch_exclude:
                        instance.add_filter("In test case arch exclude", Filters.TESTSUITE)

                    if ts.platform_exclude and plat.name in ts.platform_exclude:
                        instance.add_filter("In test case platform exclude", Filters.TESTSUITE)

                if ts.toolchain_exclude and toolchain in ts.toolchain_exclude:
                    instance.add_filter("In test case toolchain exclude", Filters.TESTSUITE)

                if platform_filter and plat.name not in platform_filter:
                    instance.add_filter("Command line platform filter", Filters.CMD_LINE)

                if ts.platform_allow and plat.name not in ts.platform_allow:
                    instance.add_filter("Not in testsuite platform allow list", Filters.TESTSUITE)

                if ts.platform_type and plat.type not in ts.platform_type:
                    instance.add_filter("Not in testsuite platform type list", Filters.TESTSUITE)

                if ts.toolchain_allow and toolchain not in ts.toolchain_allow:
                    instance.add_filter("Not in testsuite toolchain allow list", Filters.TESTSUITE)

                if not plat.env_satisfied:
                    instance.add_filter("Environment ({}) not satisfied".format(", ".join(plat.env)), Filters.PLATFORM)

                if not force_toolchain \
                        and toolchain and (toolchain not in plat.supported_toolchains) \
                        and "host" not in plat.supported_toolchains \
                        and ts.type != 'unit':
                    instance.add_filter("Not supported by the toolchain", Filters.PLATFORM)

                if plat.ram < ts.min_ram:
                    instance.add_filter("Not enough RAM", Filters.PLATFORM)

                if ts.depends_on:
                    dep_intersection = ts.depends_on.intersection(set(plat.supported))
                    if dep_intersection != set(ts.depends_on):
                        instance.add_filter("No hardware support", Filters.PLATFORM)

                if plat.flash < ts.min_flash:
                    instance.add_filter("Not enough FLASH", Filters.PLATFORM)

                if set(plat.ignore_tags) & ts.tags:
                    instance.add_filter("Excluded tags per platform (exclude_tags)", Filters.PLATFORM)

                if plat.only_tags and not set(plat.only_tags) & ts.tags:
                    instance.add_filter("Excluded tags per platform (only_tags)", Filters.PLATFORM)

                # platform_key is a list of unique platform attributes that form a unique key a test
                # will match against to determine if it should be scheduled to run. A key containing a
                # field name that the platform does not have will filter the platform.
                #
                # A simple example is keying on arch and simulation to run a test once per unique (arch, simulation) platform.
                if not ignore_platform_key and hasattr(ts, 'platform_key') and len(ts.platform_key) > 0:
                    # form a key by sorting the key fields first, then fetching the key fields from plat if they exist
                    # if a field does not exist the test is still scheduled on that platform as its undeterminable.
                    key_fields = sorted(set(ts.platform_key))
                    key = [getattr(plat, key_field) for key_field in key_fields]
                    has_all_fields = True
                    for key_field in key_fields:
                        if key_field is None or key_field == 'na':
                            has_all_fields = False
                    if has_all_fields:
                        test_key = copy.deepcopy(key)
                        test_key.append(ts.name)
                        test_key = tuple(test_key)
                        keyed_test = keyed_tests.get(test_key)
                        if keyed_test is not None:
                            plat_key = {key_field: getattr(keyed_test['plat'], key_field) for key_field in key_fields}
                            instance.add_filter(f"Excluded test already covered for key {tuple(key)} by platform {keyed_test['plat'].name} having key {plat_key}", Filters.TESTSUITE)
                        else:
                            keyed_tests[test_key] = {'plat': plat, 'ts': ts}
                    else:
                        instance.add_filter(f"Excluded platform missing key fields demanded by test {key_fields}", Filters.PLATFORM)

                # handle quarantined tests
                if self.quarantine:
                    matched_quarantine = self.quarantine.get_matched_quarantine(
                        instance.testsuite.id, plat.name, plat.arch, plat.simulation
                    )
                    if matched_quarantine and not self.options.quarantine_verify:
                        instance.add_filter("Quarantine: " + matched_quarantine, Filters.QUARENTINE)
                    if not matched_quarantine and self.options.quarantine_verify:
                        instance.add_filter("Not under quarantine", Filters.QUARENTINE)

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
                    a = set(self.default_platforms)
                    b = set(ts.platform_allow)
                    c = a.intersection(b)
                    if c:
                        aa = list(filter(lambda ts: ts.platform.name in c, instance_list))
                        self.add_instances(aa)
                    else:
                        self.add_instances(instance_list)
                else:
                    instances = list(filter(lambda ts: ts.platform.default, instance_list))
                    self.add_instances(instances)
            elif integration:
                instances = list(filter(lambda item:  item.platform.name in ts.integration_platforms, instance_list))
                self.add_instances(instances)

            elif emulation_platforms:
                self.add_instances(instance_list)
                for instance in list(filter(lambda inst: not inst.platform.simulation != 'na', instance_list)):
                    instance.add_filter("Not an emulated platform", Filters.PLATFORM)
            else:
                self.add_instances(instance_list)

        for _, case in self.instances.items():
            case.create_overlay(case.platform, self.options.enable_asan, self.options.enable_ubsan, self.options.enable_coverage, self.options.coverage_platform)

        self.selected_platforms = set(p.platform.name for p in self.instances.values())

        filtered_instances = list(filter(lambda item:  item.status == "filtered", self.instances.values()))
        for filtered_instance in filtered_instances:
            # If integration mode is on all skips on integration_platforms are treated as errors.
            if self.options.integration and filtered_instance.platform.name in filtered_instance.testsuite.integration_platforms \
                and "quarantine" not in filtered_instance.reason.lower():
                # Do not treat this as error if filter type is command line
                filters = {t['type'] for t in filtered_instance.filters}
                if Filters.CMD_LINE in filters or Filters.SKIP in filters:
                    continue
                filtered_instance.status = "error"
                filtered_instance.reason += " but is one of the integration platforms"
                self.instances[filtered_instance.name] = filtered_instance

            filtered_instance.add_missing_case_status(filtered_instance.status)

        self.filtered_platforms = set(p.platform.name for p in self.instances.values()
                                      if p.status != "skipped" )

    def add_instances(self, instance_list):
        for instance in instance_list:
            self.instances[instance.name] = instance


    def get_testsuite(self, identifier):
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
        for platform in platform_names_to_verify:
            if platform in self.platform_names:
                continue
            else:
                logger.error(f"{log_info} - unrecognized platform - {platform}")
                sys.exit(2)

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
            if instance.status != "skipped":
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
            command = ["mklink", "/J", f"{link_path}", f"{instance.build_dir}"]
            subprocess.call(command, shell=True)
        else:  # for Linux and MAC OS
            os.symlink(instance.build_dir, link_path)

        # Here original build directory is replaced with symbolic link. It will
        # be passed to CMake command
        instance.build_dir = link_path

        self.link_dir_counter += 1
