#!/usr/bin/env python3

# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0


import logging
from typing import Union, List, Dict
from pathlib import Path
import os
import argparse
import re
import json
import copy

logging.basicConfig(format='%(levelname)s: %(message)s', level=logging.DEBUG)
logger = logging.getLogger(__name__)


def parse_arguments():
    my_parser = argparse.ArgumentParser(
        description="Program collects input Twister json reports and merge them "
                    "into one final report. It can be also applied specific "
                    "tests filtration. By default, if no input is provided, "
                    "program tries to get source \"twister.json\" report from "
                    "current directory and no test filtration will be applied. "
                    "If no output folder is pointed, default destination is "
                    "\"./merged/\" directory. During comparing two test ids, "
                    "differences between path separators (\"/\" or \"\\\") are "
                    "ignored so, for example for this script following test ids "
                    "are the same: "
                    "\"native_posix/tests/kernel/poll/kernel.poll\" "
                    "\"native_posix\\tests\\kernel\\poll\\kernel.poll\"."
    )

    my_parser.add_argument(
        "--source-report", action="append", default=[],
        help="Path to source Twister report which will be used to create "
             "final merged report. This option can be used to indicate directly "
             "path to source Twister report. Example of SOURCE_REPORT: "
             "\"~/my_reports/twister.json\". This option can be use multiple "
             "times in one call."
    )

    my_parser.add_argument(
        "--source-report-base", default="",
        help="Not full path to source Twister report which will be used to "
             "generate list of available reports. For example, for "
             "SOURCE_REPORT_BASE set as \"~/my_reports/twister\" following "
             "source report paths could be generated: "
             "\"~/my_reports/twister-1.json\", "
             "\"~/my_reports/twister-2.json\", etc."
    )

    my_parser.add_argument(
        "--source-dir-base", default="",
        help="Not full path to directory which will be used to generate list "
             "of directories where Twister reports are stored. For example for "
             "SOURCE_DIR_BASE set as \"~/twister-out\" following source "
             "directories will be generated:  \"~/twister-out.1\", "
             "\"~/twister-out.2\", etc. Then program will try to get "
             "\"twister.json\" reports from each of those directories."
    )

    my_parser.add_argument(
        "-T", "--test-id", action="append", default=[],
        help="Test identifier (platform + test path + test scenario name) "
             "used for filter test suites from input reports to final report. "
             "Example of TEST_ID: \"native_posix/tests/kernel/poll/kernel.poll\""
             ". This option can be use multiple times in one call."
    )

    my_parser.add_argument(
        "--test-id-file", default="",
        help="Path to txt file, which contains list of TEST_ID (platform + "
             "test path + test scenario name) collected in separate lines used "
             "for filter test suites from input reports to final report. "
             "Example of TEST_ID: "
             "\"native_posix/tests/kernel/poll/kernel.poll\"."
    )

    my_parser.add_argument(
        "--testplan", default="",
        help="Path to testplan.json file, which contains list of test suites "
             "which will be used for filter test suites from input reports to "
             "final report. Test suites with \"status\" set as \"filtered\" will "
             "be skipped."
    )

    my_parser.add_argument(
        "--use-local-separator", action="store_true", default=False,
        help="Use local separator (\"/\" on Linux or \"\\\" on Windows) in "
             "final report in test id paths."
    )

    my_parser.add_argument(
        "--outdir", default="",
        help="Output directory where final merged report will be placed."
    )

    return my_parser.parse_args()


class TestsuiteInfo():
    def __init__(self, test_id: str):
        """
        test_id has a following format: "platform + test path + test scenario
        name", for example: "native_posix/tests/kernel/poll/kernel.poll"
        """
        self.test_id: str = test_id
        platform, test_path_name = re.split(r"/|\\", test_id, maxsplit=1)
        self.test_path_name: str = test_path_name
        self.platform: str = platform
        self.report_path: Union[Path, None] = None


class TwisterReportsMergeException(Exception):
    pass


class TwisterReportsMerge():
    default_report_name = "twister.json"
    testplan_name = "testplan.json"
    json_suffix = ".json"
    testsuite_status_filtered = "filtered"
    default_output_dir = "merged"

    def __init__(self, user_args):
        self.user_args = user_args
        self.twister_report_paths: List[Path] = []
        self.desired_testsuites: List[TestsuiteInfo] = []
        self.final_report: Dict = {}
        self.output_dir: Path = self._determine_output_dir()

    def _determine_output_dir(self) -> Path:
        output_dir = self.user_args.outdir or self.default_output_dir
        output_dir = Path(output_dir).resolve()
        if not output_dir.exists():
            output_dir.mkdir()
            logger.info("New output directory created: %s", str(output_dir))
        return output_dir

    def collect_reports(self):
        args = self.user_args
        if args.source_report:
            self._collect_reports_from_source_report()

        if args.source_report_base:
            self._collect_reports_from_source_report_base()

        if args.source_dir_base:
            self._collect_reports_from_source_dir_base()

        if not self.twister_report_paths:
            # by default if no source report is found yet, try get twister report from current directory
            self._collect_reports_from_local_dir()

        self._verify_twister_reports_existence()
        self._print_source_report_info()

    def _collect_reports_from_source_report(self):
        self.twister_report_paths += [Path(report).resolve() for report in self.user_args.source_report]

    def _collect_reports_from_source_report_base(self):
        report_base_path = Path(self.user_args.source_report_base).resolve()
        report_base_dir = report_base_path.parent
        report_base_name = report_base_path.name
        reports_found_flag = False
        for item in report_base_dir.iterdir():
            if item.is_file() and item.name.startswith(report_base_name) and item.suffix == self.json_suffix:
                self.twister_report_paths.append(item)
                reports_found_flag = True
        if not reports_found_flag:
            logger.warning("No reports starting from \"%s\" found", str(report_base_path))

    def _collect_reports_from_source_dir_base(self):
        source_dir_base_path = Path(self.user_args.source_dir_base).resolve()
        source_dir_parent = source_dir_base_path.parent
        source_dir_base_name = source_dir_base_path.name
        for item in source_dir_parent.iterdir():
            if item.is_dir() and item.name.startswith(source_dir_base_name):
                twister_report_path = item / self.default_report_name
                if twister_report_path.exists():
                    self.twister_report_paths.append(twister_report_path)
                else:
                    logger.warning("No %s found in directory: %s", self.default_report_name, str(item))

    def _collect_reports_from_local_dir(self):
        report_path = Path(self.default_report_name).resolve()
        if report_path.exists():
            self.twister_report_paths.append(report_path)
            logger.info("Get source report from local directory.")

    def _verify_twister_reports_existence(self):
        if not self.twister_report_paths:
            logger.error("No source Twister report provided.")
            raise TwisterReportsMergeException

        for report_path in self.twister_report_paths:
            if not report_path.is_file():
                logger.error("Report does not exist: %s.", str(report_path))
                raise TwisterReportsMergeException

    def _print_source_report_info(self):
        logger.info("Following Twister reports will used as source reports:")
        for report_path in self.twister_report_paths:
            logger.info("%s", str(report_path))

    def collect_filters(self):
        args = self.user_args
        if args.test_id:
            self._collect_filters_from_test_id()

        if args.test_id_file:
            self._collect_filters_from_test_id_file()

        if args.testplan:
            self._collect_filters_from_testplan()

        self._print_filters_info()

    def _is_testsuite_on_filtration_list(self, new_testsuite: TestsuiteInfo) -> bool:
        for testsuite in self.desired_testsuites:
            if testsuite.platform == new_testsuite.platform and \
                    testsuite.test_path_name == new_testsuite.test_path_name:
                logger.warning("Testsuite \"%s\" already exists on filtration list.", testsuite.test_id)
                return True
        return False

    def _collect_filters_from_test_id(self):
        for test_id in self.user_args.test_id:
            testsuite_info = TestsuiteInfo(test_id)
            if not self._is_testsuite_on_filtration_list(testsuite_info):
                self.desired_testsuites.append(testsuite_info)

    def _collect_filters_from_test_id_file(self):
        test_id_file_path = Path(self.user_args.test_id_file).resolve()
        if not test_id_file_path.exists():
            logger.error('Txt file with list of test ids does not exist %s', str(test_id_file_path))
            raise TwisterReportsMergeException
        logger.info('Collect test id from file: %s', str(test_id_file_path))
        with open(test_id_file_path, "r") as file:
            for test_id in file:
                test_id = test_id.strip()
                if test_id:
                    testsuite_info = TestsuiteInfo(test_id)
                    if not self._is_testsuite_on_filtration_list(testsuite_info):
                        self.desired_testsuites.append(testsuite_info)

    def _collect_filters_from_testplan(self):
        testplan_path = Path(self.user_args.testplan).resolve()
        if not testplan_path.exists():
            logger.error('Testplan file does not exist %s', str(testplan_path))
            raise TwisterReportsMergeException
        logger.info('Collect test id from testplan: %s', str(testplan_path))
        testplan = self._load_twister_report(testplan_path)
        try:
            testplan_testsuites = testplan["testsuites"]
        except KeyError:
            logger.error('"testsuites" not found in testplan %s', str(testplan_path))
            raise
        for testsuite in testplan_testsuites:
            testsuite_status = testsuite.get("status", "")
            if testsuite_status == self.testsuite_status_filtered:
                continue
            try:
                testsuite_name = testsuite["name"]
                testsuite_platform = testsuite["platform"]
            except KeyError:
                logger.error('"name" or "platform" options not found in testsuite in testplan: %s', str(testplan_path))
                raise
            test_id = f"{testsuite_platform}{os.path.sep}{testsuite_name}"
            testsuite_info = TestsuiteInfo(test_id)
            if not self._is_testsuite_on_filtration_list(testsuite_info):
                self.desired_testsuites.append(testsuite_info)

    def _print_filters_info(self):
        if self.desired_testsuites:
            logger.info("Following testsuite filtration will be applied:")
            for desired_testsuite in self.desired_testsuites:
                logger.info("%s", desired_testsuite.test_id)
        else:
            logger.info("No testsuite filtration applied. All testsuites from "
                        "source reports will be placed into final report.")

    def create_merged_report(self):
        # initialize final report by first report
        twister_report = self._load_twister_report(self.twister_report_paths[0])
        self._initialize_final_report(twister_report)

        for report_path in self.twister_report_paths:
            twister_report = self._load_twister_report(report_path)
            try:
                report_testsuites = twister_report["testsuites"]
            except KeyError:
                logger.error('"testsuites" not found in report %s', str(report_path))
                raise
            self._apply_filtration(report_testsuites, report_path)

        self._verify_desired_testsuites()
        if self.user_args.use_local_separator:
            self._set_local_separator_in_final_report()
        self._save_final_report()

    @staticmethod
    def _load_twister_report(report_path: Path):
        with open(report_path, "r") as file:
            twister_report = json.loads(file.read())
        return twister_report

    def _initialize_final_report(self, twister_report: Dict):
        self.final_report = copy.deepcopy(twister_report)
        self.final_report["testsuites"] = []

    def _apply_filtration(self, report_testsuites: List[Dict], report_path: Path):
        if self.desired_testsuites:
            for desired_testsuite in self.desired_testsuites:
                for report_testsuite in report_testsuites:
                    self._compare_testsuites(desired_testsuite, report_testsuite, report_path)
        else:  # no filtration applied
            self.final_report["testsuites"] += report_testsuites

    def _compare_testsuites(self, desired_testsuite: TestsuiteInfo, report_testsuite: Dict, report_path: Path):
        try:
            report_testsuite_name = report_testsuite["name"]
            report_testsuite_platform = report_testsuite["platform"]
        except KeyError:
            logger.error('"name" or "platform" options not found in testsuite in report: %s', str(report_path))
            raise
        report_testsuite_id = f"{report_testsuite_platform}{os.path.sep}{report_testsuite_name}"
        if self._compare_test_id(desired_testsuite.test_id, report_testsuite_id):
            if desired_testsuite.report_path:
                logger.warning(
                    "Testsuite %s found in two reports: \n%s \n%s",
                    desired_testsuite.test_id, str(desired_testsuite.report_path), str(report_path)
                )
            else:
                desired_testsuite.report_path = report_path
                self.final_report["testsuites"].append(report_testsuite)
                logger.debug("Testsuite %s found in report \"%s\"", desired_testsuite.test_id, str(report_path))

    @staticmethod
    def _compare_test_id(test_id_1: str, test_id_2: str) -> bool:
        test_id_1_elements = re.split(r"/|\\", test_id_1)
        test_id_2_elements = re.split(r"/|\\", test_id_2)
        return test_id_1_elements == test_id_2_elements

    def _verify_desired_testsuites(self):
        if self.desired_testsuites:
            for desired_testsuite in self.desired_testsuites:
                if desired_testsuite.report_path is None:
                    logger.error("Testsuite \"%s\" not found", desired_testsuite.test_id)
                    raise TwisterReportsMergeException
        else:
            pass

    def _set_local_separator_in_final_report(self):
        local_sep = os.path.sep
        logger.info("Set local separator \"%s\" as separator in test ids in "
                    "final report.", local_sep)
        for testsuite in self.final_report["testsuites"]:
            test_path_name = testsuite["name"]
            test_path_name = re.sub(r"/|\\", local_sep, test_path_name)
            testsuite["name"] = test_path_name

    def _save_final_report(self):
        final_report_path = self.output_dir / self.default_report_name
        with open(final_report_path, 'w', encoding='utf-8') as file:
            json.dump(self.final_report, file, indent=4, separators=(',', ':'))
        logger.info("Final report saved into: %s", str(final_report_path))


def main():
    user_args = parse_arguments()
    reports_merge = TwisterReportsMerge(user_args)
    reports_merge.collect_reports()
    reports_merge.collect_filters()
    reports_merge.create_merged_report()


if __name__ == "__main__":
    main()
