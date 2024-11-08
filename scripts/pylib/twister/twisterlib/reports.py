#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

from enum import Enum
import os
import json
import logging
from colorama import Fore
import xml.etree.ElementTree as ET
import string
from datetime import datetime
from pathlib import PosixPath

from twisterlib.statuses import TwisterStatus

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)


class ReportStatus(str, Enum):
    def __str__(self):
        return str(self.value)

    ERROR = 'error'
    FAIL = 'failure'
    SKIP = 'skipped'


class Reporting:

    json_filters = {
        'twister.json': {
            'deny_suite': ['footprint']
        },
        'footprint.json': {
            'deny_status': ['FILTER'],
            'deny_suite': ['testcases', 'execution_time', 'recording', 'retries', 'runnable']
        }
    }

    def __init__(self, plan, env) -> None:
        self.plan = plan #FIXME
        self.instances = plan.instances
        self.platforms = plan.platforms
        self.selected_platforms = plan.selected_platforms
        self.filtered_platforms = plan.filtered_platforms
        self.env = env
        self.timestamp = datetime.now().isoformat()
        self.outdir = os.path.abspath(env.options.outdir)
        self.instance_fail_count = plan.instance_fail_count
        self.footprint = None

    @staticmethod
    def process_log(log_file):
        filtered_string = ""
        if os.path.exists(log_file):
            with open(log_file, "rb") as f:
                log = f.read().decode("utf-8")
                filtered_string = ''.join(filter(lambda x: x in string.printable, log))

        return filtered_string


    @staticmethod
    def xunit_testcase(eleTestsuite, name, classname, status: TwisterStatus, ts_status: TwisterStatus, reason, duration, runnable, stats, log, build_only_as_skip):
        fails, passes, errors, skips = stats

        if status in [TwisterStatus.SKIP, TwisterStatus.FILTER]:
            duration = 0

        eleTestcase = ET.SubElement(
            eleTestsuite, "testcase",
            classname=classname,
            name=f"{name}",
            time=f"{duration}")

        if status in [TwisterStatus.SKIP, TwisterStatus.FILTER]:
            skips += 1
            # temporarily add build_only_as_skip to restore existing CI report behaviour
            if ts_status == TwisterStatus.PASS and not runnable:
                tc_type = "build"
            else:
                tc_type = status
            ET.SubElement(eleTestcase, ReportStatus.SKIP, type=f"{tc_type}", message=f"{reason}")
        elif status in [TwisterStatus.FAIL, TwisterStatus.BLOCK]:
            fails += 1
            el = ET.SubElement(eleTestcase, ReportStatus.FAIL, type="failure", message=f"{reason}")
            if log:
                el.text = log
        elif status == TwisterStatus.ERROR:
            errors += 1
            el = ET.SubElement(eleTestcase, ReportStatus.ERROR, type="failure", message=f"{reason}")
            if log:
                el.text = log
        elif status == TwisterStatus.PASS:
            passes += 1
        elif status == TwisterStatus.NOTRUN:
            if build_only_as_skip:
                ET.SubElement(eleTestcase, ReportStatus.SKIP, type="build", message="built only")
                skips += 1
            else:
                passes += 1
        else:
            if status == TwisterStatus.NONE:
                logger.debug(f"{name}: No status")
                ET.SubElement(eleTestcase, ReportStatus.SKIP, type=f"untested", message="No results captured, testsuite misconfiguration?")
            else:
                logger.error(f"{name}: Unknown status '{status}'")

        return (fails, passes, errors, skips)

    # Generate a report with all testsuites instead of doing this per platform
    def xunit_report_suites(self, json_file, filename):

        json_data = {}
        with open(json_file, "r") as json_results:
            json_data = json.load(json_results)


        env = json_data.get('environment', {})
        version = env.get('zephyr_version', None)

        eleTestsuites = ET.Element('testsuites')
        all_suites = json_data.get("testsuites", [])

        suites_to_report = all_suites
            # do not create entry if everything is filtered out
        if not self.env.options.detailed_skipped_report:
            suites_to_report = list(filter(lambda d: TwisterStatus(d.get('status')) != TwisterStatus.FILTER, all_suites))

        for suite in suites_to_report:
            duration = 0
            eleTestsuite = ET.SubElement(eleTestsuites, 'testsuite',
                                            name=suite.get("name"), time="0",
                                            timestamp = self.timestamp,
                                            tests="0",
                                            failures="0",
                                            errors="0", skipped="0")
            eleTSPropetries = ET.SubElement(eleTestsuite, 'properties')
            # Multiple 'property' can be added to 'properties'
            # differing by name and value
            ET.SubElement(eleTSPropetries, 'property', name="version", value=version)
            ET.SubElement(eleTSPropetries, 'property', name="platform", value=suite.get("platform"))
            ET.SubElement(eleTSPropetries, 'property', name="architecture", value=suite.get("arch"))

            total = 0
            fails = passes = errors = skips = 0
            handler_time = suite.get('execution_time', 0)
            runnable = suite.get('runnable', 0)
            duration += float(handler_time)
            ts_status = TwisterStatus(suite.get('status'))
            for tc in suite.get("testcases", []):
                status = TwisterStatus(tc.get('status'))
                reason = tc.get('reason', suite.get('reason', 'Unknown'))
                log = tc.get("log", suite.get("log"))

                tc_duration = tc.get('execution_time', handler_time)
                name = tc.get("identifier")
                classname = ".".join(name.split(".")[:2])
                fails, passes, errors, skips = self.xunit_testcase(eleTestsuite,
                    name, classname, status, ts_status, reason, tc_duration, runnable,
                    (fails, passes, errors, skips), log, True)

            total = errors + passes + fails + skips

            eleTestsuite.attrib['time'] = f"{duration}"
            eleTestsuite.attrib['failures'] = f"{fails}"
            eleTestsuite.attrib['errors'] = f"{errors}"
            eleTestsuite.attrib['skipped'] = f"{skips}"
            eleTestsuite.attrib['tests'] = f"{total}"

        result = ET.tostring(eleTestsuites)
        with open(filename, 'wb') as report:
            report.write(result)

    def xunit_report(self, json_file, filename, selected_platform=None, full_report=False):
        if selected_platform:
            selected = [selected_platform]
            logger.info(f"Writing target report for {selected_platform}...")
        else:
            logger.info(f"Writing xunit report {filename}...")
            selected = self.selected_platforms

        json_data = {}
        with open(json_file, "r") as json_results:
            json_data = json.load(json_results)


        env = json_data.get('environment', {})
        version = env.get('zephyr_version', None)

        eleTestsuites = ET.Element('testsuites')
        all_suites = json_data.get("testsuites", [])

        for platform in selected:
            suites = list(filter(lambda d: d['platform'] == platform, all_suites))
            # do not create entry if everything is filtered out
            if not self.env.options.detailed_skipped_report:
                non_filtered = list(filter(lambda d: TwisterStatus(d.get('status')) != TwisterStatus.FILTER, suites))
                if not non_filtered:
                    continue

            duration = 0
            eleTestsuite = ET.SubElement(eleTestsuites, 'testsuite',
                                            name=platform,
                                            timestamp = self.timestamp,
                                            time="0",
                                            tests="0",
                                            failures="0",
                                            errors="0", skipped="0")
            eleTSPropetries = ET.SubElement(eleTestsuite, 'properties')
            # Multiple 'property' can be added to 'properties'
            # differing by name and value
            ET.SubElement(eleTSPropetries, 'property', name="version", value=version)

            total = 0
            fails = passes = errors = skips = 0
            for ts in suites:
                handler_time = ts.get('execution_time', 0)
                runnable = ts.get('runnable', 0)
                duration += float(handler_time)

                ts_status = TwisterStatus(ts.get('status'))
                # Do not report filtered testcases
                if ts_status == TwisterStatus.FILTER and not self.env.options.detailed_skipped_report:
                    continue
                if full_report:
                    for tc in ts.get("testcases", []):
                        status = TwisterStatus(tc.get('status'))
                        reason = tc.get('reason', ts.get('reason', 'Unknown'))
                        log = tc.get("log", ts.get("log"))

                        tc_duration = tc.get('execution_time', handler_time)
                        name = tc.get("identifier")
                        classname = ".".join(name.split(".")[:2])
                        fails, passes, errors, skips = self.xunit_testcase(eleTestsuite,
                            name, classname, status, ts_status, reason, tc_duration, runnable,
                            (fails, passes, errors, skips), log, True)
                else:
                    reason = ts.get('reason', 'Unknown')
                    name = ts.get("name")
                    classname = f"{platform}:{name}"
                    log = ts.get("log")
                    fails, passes, errors, skips = self.xunit_testcase(eleTestsuite,
                        name, classname, ts_status, ts_status, reason, handler_time, runnable,
                        (fails, passes, errors, skips), log, False)

            total = errors + passes + fails + skips

            eleTestsuite.attrib['time'] = f"{duration}"
            eleTestsuite.attrib['failures'] = f"{fails}"
            eleTestsuite.attrib['errors'] = f"{errors}"
            eleTestsuite.attrib['skipped'] = f"{skips}"
            eleTestsuite.attrib['tests'] = f"{total}"

        result = ET.tostring(eleTestsuites)
        with open(filename, 'wb') as report:
            report.write(result)

    def json_report(self, filename, version="NA", platform=None, filters=None):
        logger.info(f"Writing JSON report {filename}")

        if self.env.options.report_all_options:
            report_options = vars(self.env.options)
        else:
            report_options = self.env.non_default_options()

        # Resolve known JSON serialization problems.
        for k,v in report_options.items():
            report_options[k] = str(v) if type(v) in [PosixPath] else v

        report = {}
        report["environment"] = {"os": os.name,
                                 "zephyr_version": version,
                                 "toolchain": self.env.toolchain,
                                 "commit_date": self.env.commit_date,
                                 "run_date": self.env.run_date,
                                 "options": report_options
                                 }
        suites = []

        for instance in self.instances.values():
            if platform and platform != instance.platform.name:
                continue
            if instance.status == TwisterStatus.FILTER and not self.env.options.report_filtered:
                continue
            if (filters and 'allow_status' in filters and \
                instance.status not in [TwisterStatus[s] for s in filters['allow_status']]):
                logger.debug(f"Skip test suite '{instance.testsuite.name}' status '{instance.status}' "
                             f"not allowed for {filename}")
                continue
            if (filters and 'deny_status' in filters and \
                instance.status in [TwisterStatus[s] for s in filters['deny_status']]):
                logger.debug(f"Skip test suite '{instance.testsuite.name}' status '{instance.status}' "
                             f"denied for {filename}")
                continue
            suite = {}
            handler_log = os.path.join(instance.build_dir, "handler.log")
            pytest_log = os.path.join(instance.build_dir, "twister_harness.log")
            build_log = os.path.join(instance.build_dir, "build.log")
            device_log = os.path.join(instance.build_dir, "device.log")

            handler_time = instance.metrics.get('handler_time', 0)
            used_ram = instance.metrics.get ("used_ram", 0)
            used_rom  = instance.metrics.get("used_rom",0)
            available_ram = instance.metrics.get("available_ram", 0)
            available_rom = instance.metrics.get("available_rom", 0)
            suite = {
                "name": instance.testsuite.name,
                "arch": instance.platform.arch,
                "platform": instance.platform.name,
                "path": instance.testsuite.source_dir_rel
            }
            if instance.run_id:
                suite['run_id'] = instance.run_id

            suite["runnable"] = False
            if instance.status != TwisterStatus.FILTER:
                suite["runnable"] = instance.run

            if used_ram:
                suite["used_ram"] = used_ram
            if used_rom:
                suite["used_rom"] = used_rom

            suite['retries'] = instance.retries

            if instance.dut:
                suite["dut"] = instance.dut
            if available_ram:
                suite["available_ram"] = available_ram
            if available_rom:
                suite["available_rom"] = available_rom
            if instance.status in [TwisterStatus.ERROR, TwisterStatus.FAIL]:
                suite['status'] = instance.status
                suite["reason"] = instance.reason
                # FIXME
                if os.path.exists(pytest_log):
                    suite["log"] = self.process_log(pytest_log)
                elif os.path.exists(handler_log):
                    suite["log"] = self.process_log(handler_log)
                elif os.path.exists(device_log):
                    suite["log"] = self.process_log(device_log)
                else:
                    suite["log"] = self.process_log(build_log)
            elif instance.status == TwisterStatus.FILTER:
                suite["status"] = TwisterStatus.FILTER
                suite["reason"] = instance.reason
            elif instance.status == TwisterStatus.PASS:
                suite["status"] = TwisterStatus.PASS
            elif instance.status == TwisterStatus.SKIP:
                suite["status"] = TwisterStatus.SKIP
                suite["reason"] = instance.reason
            elif instance.status == TwisterStatus.NOTRUN:
                suite["status"] = TwisterStatus.NOTRUN
                suite["reason"] = instance.reason
            else:
                suite["status"] = TwisterStatus.NONE
                suite["reason"] = 'Unknown Instance status.'

            if instance.status != TwisterStatus.NONE:
                suite["execution_time"] =  f"{float(handler_time):.2f}"
            suite["build_time"] =  f"{float(instance.build_time):.2f}"

            testcases = []

            if len(instance.testcases) == 1:
                single_case_duration = f"{float(handler_time):.2f}"
            else:
                single_case_duration = 0

            for case in instance.testcases:
                # freeform was set when no sub testcases were parsed, however,
                # if we discover those at runtime, the fallback testcase wont be
                # needed anymore and can be removed from the output, it does
                # not have a status and would otherwise be reported as skipped.
                if case.freeform and case.status == TwisterStatus.NONE and len(instance.testcases) > 1:
                    continue
                testcase = {}
                testcase['identifier'] = case.name
                if instance.status != TwisterStatus.NONE:
                    if single_case_duration:
                        testcase['execution_time'] = single_case_duration
                    else:
                        testcase['execution_time'] = f"{float(case.duration):.2f}"

                if case.output != "":
                    testcase['log'] = case.output

                if case.status == TwisterStatus.SKIP:
                    if instance.status == TwisterStatus.FILTER:
                        testcase["status"] = TwisterStatus.FILTER
                    else:
                        testcase["status"] = TwisterStatus.SKIP
                        testcase["reason"] = case.reason or instance.reason
                else:
                    testcase["status"] = case.status
                    if case.reason:
                        testcase["reason"] = case.reason

                testcases.append(testcase)

            suite['testcases'] = testcases

            if instance.recording is not None:
                suite['recording'] = instance.recording

            if (instance.status not in [TwisterStatus.NONE, TwisterStatus.ERROR, TwisterStatus.FILTER]
                    and self.env.options.create_rom_ram_report
                    and self.env.options.footprint_report is not None):
                # Init as empty data preparing for filtering properties.
                suite['footprint'] = {}

            # Pass suite properties through the context filters.
            if filters and 'allow_suite' in filters:
                suite = {k:v for k,v in suite.items() if k in filters['allow_suite']}

            if filters and 'deny_suite' in filters:
                suite = {k:v for k,v in suite.items() if k not in filters['deny_suite']}

            # Compose external data only to these properties which pass filtering.
            if 'footprint' in suite:
                do_all = 'all' in self.env.options.footprint_report
                footprint_files = { 'ROM': 'rom.json', 'RAM': 'ram.json' }
                for k,v in footprint_files.items():
                    if do_all or k in self.env.options.footprint_report:
                        footprint_fname = os.path.join(instance.build_dir, v)
                        try:
                            with open(footprint_fname, "rt") as footprint_json:
                                logger.debug(f"Collect footprint.{k} for '{instance.name}'")
                                suite['footprint'][k] = json.load(footprint_json)
                        except FileNotFoundError:
                            logger.error(f"Missing footprint.{k} for '{instance.name}'")
                #
            #

            suites.append(suite)

        report["testsuites"] = suites
        with open(filename, "wt") as json_file:
            json.dump(report, json_file, indent=4, separators=(',',':'))


    def compare_metrics(self, filename):
        # name, datatype, lower results better
        interesting_metrics = [("used_ram", int, True),
                               ("used_rom", int, True)]

        if not os.path.exists(filename):
            logger.error("Cannot compare metrics, %s not found" % filename)
            return []

        results = []
        saved_metrics = {}
        with open(filename) as fp:
            jt = json.load(fp)
            for ts in jt.get("testsuites", []):
                d = {}
                for m, _, _ in interesting_metrics:
                    d[m] = ts.get(m, 0)
                ts_name = ts.get('name')
                ts_platform = ts.get('platform')
                saved_metrics[(ts_name, ts_platform)] = d

        for instance in self.instances.values():
            mkey = (instance.testsuite.name, instance.platform.name)
            if mkey not in saved_metrics:
                continue
            sm = saved_metrics[mkey]
            for metric, mtype, lower_better in interesting_metrics:
                if metric not in instance.metrics:
                    continue
                if sm[metric] == "":
                    continue
                delta = instance.metrics.get(metric, 0) - mtype(sm[metric])
                if delta == 0:
                    continue
                results.append((instance, metric, instance.metrics.get(metric, 0), delta,
                                lower_better))
        return results

    def footprint_reports(self, report, show_footprint, all_deltas,
                          footprint_threshold, last_metrics):
        if not report:
            return

        logger.debug("running footprint_reports")
        deltas = self.compare_metrics(report)
        warnings = 0
        if deltas:
            for i, metric, value, delta, lower_better in deltas:
                if not all_deltas and ((delta < 0 and lower_better) or
                                       (delta > 0 and not lower_better)):
                    continue

                percentage = 0
                if value > delta:
                    percentage = (float(delta) / float(value - delta))

                if not all_deltas and (percentage < (footprint_threshold / 100.0)):
                    continue

                if show_footprint:
                    logger.log(
                        logging.INFO if all_deltas else logging.WARNING,
                        "{:<25} {:<60} {} {:<+4}, is now {:6} {:+.2%}".format(
                        i.platform.name, i.testsuite.name,
                        metric, delta, value, percentage))

                warnings += 1

        if warnings:
            logger.warning("Found {} footprint deltas to {} as a baseline.".format(
                           warnings,
                           (report if not last_metrics else "the last twister run.")))

    def synopsis(self):
        if self.env.options.report_summary == 0:
            count = self.instance_fail_count
            log_txt = f"The following issues were found (showing the all {count} items):"
        elif self.env.options.report_summary:
            count = self.env.options.report_summary
            log_txt = f"The following issues were found "
            if count > self.instance_fail_count:
                log_txt += f"(presenting {self.instance_fail_count} out of the {count} items requested):"
            else:
                log_txt += f"(showing the {count} of {self.instance_fail_count} items):"
        else:
            count = 10
            log_txt = f"The following issues were found (showing the top {count} items):"
        cnt = 0
        example_instance = None
        detailed_test_id = self.env.options.detailed_test_id
        for instance in self.instances.values():
            if instance.status not in [TwisterStatus.PASS, TwisterStatus.FILTER, TwisterStatus.SKIP, TwisterStatus.NOTRUN]:
                cnt += 1
                if cnt == 1:
                    logger.info("-+" * 40)
                    logger.info(log_txt)

                status = instance.status
                if self.env.options.report_summary is not None and \
                   status in [TwisterStatus.ERROR, TwisterStatus.FAIL]:
                    status = Fore.RED + status.upper() + Fore.RESET
                logger.info(f"{cnt}) {instance.testsuite.name} on {instance.platform.name} {status} ({instance.reason})")
                example_instance = instance
            if cnt == count:
                break
        if cnt == 0 and self.env.options.report_summary is not None:
            logger.info("-+" * 40)
            logger.info(f"No errors/fails found")

        if cnt and example_instance:
            cwd_rel_path = os.path.relpath(example_instance.testsuite.source_dir, start=os.getcwd())

            logger.info("")
            logger.info("To rerun the tests, call twister using the following commandline:")
            extra_parameters = '' if detailed_test_id else ' --no-detailed-test-id'
            logger.info(f"west twister -p <PLATFORM> -s <TEST ID>{extra_parameters}, for example:")
            logger.info("")
            logger.info(f"west twister -p {example_instance.platform.name} -s {example_instance.testsuite.name}"
                        f"{extra_parameters}")
            logger.info(f"or with west:")
            logger.info(f"west build -p -b {example_instance.platform.name} {cwd_rel_path} -T {example_instance.testsuite.id}")
            logger.info("-+" * 40)

    def summary(self, results, ignore_unrecognized_sections, duration):
        failed = 0
        run = 0
        for instance in self.instances.values():
            if instance.status == TwisterStatus.FAIL:
                failed += 1
            elif not ignore_unrecognized_sections and instance.metrics.get("unrecognized"):
                logger.error("%sFAILED%s: %s has unrecognized binary sections: %s" %
                             (Fore.RED, Fore.RESET, instance.name,
                              str(instance.metrics.get("unrecognized", []))))
                failed += 1

            # FIXME: need a better way to identify executed tests
            handler_time = instance.metrics.get('handler_time', 0)
            if float(handler_time) > 0:
                run += 1

        if results.total and results.total != results.skipped_configs:
            pass_rate = (float(results.passed) / float(results.total - results.skipped_configs))
        else:
            pass_rate = 0

        logger.info(
            f"{TwisterStatus.get_color(TwisterStatus.FAIL) if failed else TwisterStatus.get_color(TwisterStatus.PASS)}{results.passed}"
            f" of {results.total - results.skipped_configs}{Fore.RESET}"
            f" executed test configurations passed ({pass_rate:.2%}),"
            f" {f'{TwisterStatus.get_color(TwisterStatus.NOTRUN)}{results.notrun}{Fore.RESET}' if results.notrun else f'{results.notrun}'} built (not run),"
            f" {f'{TwisterStatus.get_color(TwisterStatus.FAIL)}{results.failed}{Fore.RESET}' if results.failed else f'{results.failed}'} failed,"
            f" {f'{TwisterStatus.get_color(TwisterStatus.ERROR)}{results.error}{Fore.RESET}' if results.error else f'{results.error}'} errored,"
            f" with {f'{Fore.YELLOW}{self.plan.warnings + results.warnings}{Fore.RESET}' if (self.plan.warnings + results.warnings) else 'no'} warnings"
            f" in {duration:.2f} seconds."
        )

        total_platforms = len(self.platforms)
        # if we are only building, do not report about tests being executed.
        if self.platforms and not self.env.options.build_only:
            executed_cases = results.cases - results.filtered_cases - results.skipped_cases - results.notrun_cases
            pass_rate = 100 * (float(results.passed_cases) / float(executed_cases)) \
                if executed_cases != 0 else 0
            platform_rate = (100 * len(self.filtered_platforms) / len(self.platforms))
            logger.info(
                f'{results.passed_cases} of {executed_cases} executed test cases passed ({pass_rate:02.2f}%)'
                f'{", " + str(results.blocked_cases) + " blocked" if results.blocked_cases else ""}'
                f'{", " + str(results.failed_cases) + " failed" if results.failed_cases else ""}'
                f'{", " + str(results.error_cases) + " errored" if results.error_cases else ""}'
                f'{", " + str(results.none_cases) + " without a status" if results.none_cases else ""}'
                f' on {len(self.filtered_platforms)} out of total {total_platforms} platforms ({platform_rate:02.2f}%).'
            )
            if results.skipped_cases or results.notrun_cases:
                logger.info(
                    f'{results.skipped_cases + results.notrun_cases} selected test cases not executed:' \
                    f'{" " + str(results.skipped_cases) + " skipped" if results.skipped_cases else ""}' \
                    f'{(", " if results.skipped_cases else " ") + str(results.notrun_cases) + " not run (built only)" if results.notrun_cases else ""}' \
                    f'.'
                )

        built_only = results.total - run - results.skipped_configs
        logger.info(f"{Fore.GREEN}{run}{Fore.RESET} test configurations executed on platforms, \
{TwisterStatus.get_color(TwisterStatus.NOTRUN)}{built_only}{Fore.RESET} test configurations were only built.")

    def save_reports(self, name, suffix, report_dir, no_update, platform_reports):
        if not self.instances:
            return

        logger.info("Saving reports...")
        if name:
            report_name = name
        else:
            report_name = "twister"

        if report_dir:
            os.makedirs(report_dir, exist_ok=True)
            filename = os.path.join(report_dir, report_name)
            outdir = report_dir
        else:
            outdir = self.outdir
            filename = os.path.join(outdir, report_name)

        if suffix:
            filename = "{}_{}".format(filename, suffix)

        if not no_update:
            json_file = filename + ".json"
            self.json_report(json_file, version=self.env.version,
                             filters=self.json_filters['twister.json'])
            if self.env.options.footprint_report is not None:
                self.json_report(filename + "_footprint.json", version=self.env.version,
                                 filters=self.json_filters['footprint.json'])
            self.xunit_report(json_file, filename + ".xml", full_report=False)
            self.xunit_report(json_file, filename + "_report.xml", full_report=True)
            self.xunit_report_suites(json_file, filename + "_suite_report.xml")

            if platform_reports:
                self.target_report(json_file, outdir, suffix)


    def target_report(self, json_file, outdir, suffix):
        platforms = {repr(inst.platform):inst.platform for _, inst in self.instances.items()}
        for platform in platforms.values():
            if suffix:
                filename = os.path.join(outdir,"{}_{}.xml".format(platform.normalized_name, suffix))
                json_platform_file = os.path.join(outdir,"{}_{}".format(platform.normalized_name, suffix))
            else:
                filename = os.path.join(outdir,"{}.xml".format(platform.normalized_name))
                json_platform_file = os.path.join(outdir, platform.normalized_name)
            self.xunit_report(json_file, filename, platform.name, full_report=True)
            self.json_report(json_platform_file + ".json",
                             version=self.env.version, platform=platform.name,
                             filters=self.json_filters['twister.json'])
            if self.env.options.footprint_report is not None:
                self.json_report(json_platform_file + "_footprint.json",
                                 version=self.env.version, platform=platform.name,
                                 filters=self.json_filters['footprint.json'])
