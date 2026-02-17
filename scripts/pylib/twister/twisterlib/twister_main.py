# vim: set syntax=python ts=4 :
#
# Copyright (c) 2022 Google
# SPDX-License-Identifier: Apache-2.0

from __future__ import annotations

import argparse
import logging
import os
import shutil
import sys
import time
import warnings
from collections.abc import Sequence

import colorama
from colorama import Fore
from twisterlib.coverage import run_coverage
from twisterlib.environment import (
    TwisterEnv,
    add_parse_arguments,
    parse_arguments,
    python_version_guard,
)
from twisterlib.hardwaremap import HardwareMap
from twisterlib.log_helper import close_logging, setup_logging
from twisterlib.package import Artifacts
from twisterlib.reports import Reporting
from twisterlib.runner import TwisterRunner
from twisterlib.statuses import TwisterStatus
from twisterlib.testplan import TestPlan


def init_color(colorama_strip):
    colorama.init(strip=colorama_strip)


def catch_system_exit_exception(func):
    """Decorator to catch SystemExit exception."""

    def _inner(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except SystemExit as exc:
            if isinstance(exc.code, int):
                return exc.code
            if exc.code is None:
                return 0
            # if exc.code is not int/None consider it is not zero
            return 1

    return _inner


class Twister:
    """Main class for Twister."""

    def __init__(self, options: argparse.Namespace, default_options: argparse.Namespace) -> None:
        """Initialize Twister."""
        self.options: argparse.Namespace = options
        self.default_options: argparse.Namespace = default_options
        self.logger: logging.Logger = logging.getLogger("twister")
        self.start_time: float = 0.0
        self.tplan: TestPlan | None = None
        self.runner: TwisterRunner | None = None
        self.hwm: HardwareMap | None = None
        self.env: TwisterEnv | None = None
        self.report: Reporting | None = None

    def __repr__(self) -> str:
        return f'{self.__class__.__name__}()'

    @classmethod
    def create_instance(cls, argv: Sequence[str] | None = None) -> Twister:
        """Create a new instance of Twister from CLI arguments."""
        parser = add_parse_arguments()
        options = parse_arguments(parser, argv)
        default_options = parse_arguments(parser, [], on_init=False)
        return cls(options, default_options)

    def clean_previous_results(self) -> str | None:
        """Run cleanup and return a results from previous session."""
        previous_results = None
        if (
            self.options.no_clean
            or self.options.only_failed
            or self.options.test_only
            or self.options.list_tests
            or self.options.list_tags
            or self.options.test_tree
            or self.options.report_summary is not None
        ):
            if os.path.exists(self.options.outdir):
                print("Keeping artifacts untouched")
        elif self.options.last_metrics:
            ls = os.path.join(self.options.outdir, "twister.json")
            if os.path.exists(ls):
                with open(ls) as fp:
                    previous_results = fp.read()
            else:
                sys.exit(f"Can't compare metrics with non existing file {ls}")
        elif os.path.exists(self.options.outdir):
            if self.options.clobber_output:
                print(f"Deleting output directory {self.options.outdir}")
                shutil.rmtree(self.options.outdir)
            else:
                for i in range(1, 100):
                    new_out = self.options.outdir + f".{i}"
                    if not os.path.exists(new_out):
                        print(f"Renaming previous output directory to {new_out}")
                        shutil.move(self.options.outdir, new_out)
                        break
                else:
                    sys.exit(
                        f"Too many '{self.options.outdir}.*' directories. "
                        "Run either with --no-clean, "
                        "or --clobber-output, or delete these directories manually."
                    )

        return previous_results

    def configure_color_output(self) -> None:
        """Configure color output."""
        color_strip = False if self.options.force_color else None
        init_color(colorama_strip=color_strip)

    def export_previous_results(self, previous_results: str | None) -> str | None:
        """Export previous results to a file and return path to that file."""
        previous_results_file = None
        os.makedirs(self.options.outdir, exist_ok=True)
        if self.options.last_metrics and previous_results:
            previous_results_file = os.path.join(self.options.outdir, "baseline.json")
            with open(previous_results_file, "w") as fp:
                fp.write(previous_results)
        return previous_results_file

    def create_test_plan(self, env: TwisterEnv) -> TestPlan:
        """Create a test plan."""
        tplan = TestPlan(env)
        try:
            tplan.discover()
        except RuntimeError as e:
            self.logger.error(f"{e}")
            raise SystemExit(1) from e

        if tplan.report() == 0:
            raise SystemExit(0)

        try:
            tplan.load()
        except RuntimeError as e:
            self.logger.error(f"{e}")
            raise SystemExit(1) from e

        return tplan

    def discover_hardware_map(self, env: TwisterEnv) -> HardwareMap:
        """Discover hardware map and return it."""
        hwm = HardwareMap(env)
        ret = hwm.discover()
        if ret == 0:
            raise SystemExit(0)
        return hwm

    def verbose(self, tplan: TestPlan) -> None:
        """Print additional info for verbose."""
        for i in tplan.instances.values():
            if i.status in [TwisterStatus.SKIP, TwisterStatus.FILTER]:
                if (
                    self.options.platform
                    and not tplan.check_platform(i.platform, self.options.platform)
                ):
                    continue
                # Filtered tests should be visible only when verbosity > 1
                if self.options.verbose < 2 and i.status == TwisterStatus.FILTER:
                    continue
                res = i.reason
                if "Quarantine" in i.reason:
                    res = "Quarantined"
                self.logger.info(
                    f"{i.platform.name:<25} {i.testsuite.name:<50}"
                    f" {Fore.YELLOW}{i.status.upper()}{Fore.RESET}: {res}"
                )

    def create_report(self, tplan: TestPlan, env: TwisterEnv) -> Reporting:
        """Create a report."""
        report = Reporting(tplan, env)
        plan_file = os.path.join(self.options.outdir, "testplan.json")
        if not os.path.exists(plan_file):
            report.json_report(plan_file, env.version)

        if self.options.save_tests:
            report.json_report(self.options.save_tests, env.version)
            raise SystemExit(0)

        if self.options.report_summary is not None:
            if self.options.report_summary < 0:
                self.logger.error("The report summary value cannot be less than 0")
                raise SystemExit(1)
            report.synopsis()
            raise SystemExit(0)
        return report

    def execute(self, tplan: TestPlan, env: TwisterEnv, hwm: HardwareMap) -> TwisterRunner:
        """Run twister runner."""
        runner = TwisterRunner(tplan.instances, tplan.testsuites, env)
        runner.duts = hwm.duts
        runner.run()
        return runner

    def match_platforms_names(self, hwm: HardwareMap, tplan: TestPlan) -> None:
        # FIXME: This is a workaround for the fact that the hardware map can be usng
        # the short name of the platform, while the testplan is using the full name.
        #
        # convert platform names coming from the hardware map to the full target
        # name.
        # this is needed to match the platform names in the testplan.
        for d in hwm.duts:
            if d.platform in tplan.platform_names:
                d.platform = tplan.get_platform(d.platform).name

    def prepare_reports(self, report: Reporting, previous_results_file: str | None) -> None:
        """Prepare reports."""
        # figure out which report to use for size comparison
        report_to_use = None
        if self.options.compare_report:
            report_to_use = self.options.compare_report
        elif self.options.last_metrics:
            report_to_use = previous_results_file

        report.footprint_reports(
            report_to_use,
            self.options.show_footprint,
            self.options.all_deltas,
            self.options.footprint_threshold,
            self.options.last_metrics,
        )

        duration = time.time() - self.start_time

        if self.options.verbose > 1:
            self.runner.results.summary()

        report.summary(self.runner.results, duration)

        report.coverage_status = True
        if self.options.coverage and not self.options.disable_coverage_aggregation:
            if not self.options.build_only:
                report.coverage_status, report.coverage = run_coverage(self.options, self.tplan)
            else:
                self.logger.info("Skipping coverage report generation due to --build-only.")

        if self.options.device_testing and not self.options.build_only:
            self.hwm.summary(self.tplan.selected_platforms)

        report.save_reports(
            self.options.report_name,
            self.options.report_suffix,
            self.options.report_dir,
            self.options.no_update,
            self.options.platform_reports,
        )

        report.synopsis()

        if self.options.package_artifacts:
            artifacts = Artifacts(self.env)
            artifacts.package()

        if (
            self.runner.results.failed
            or self.runner.results.error
            or (self.tplan.warnings and self.options.warnings_as_errors)
            or (self.options.coverage and not report.coverage_status)
        ):
            if self.env.options.quit_on_failure:
                self.logger.info("twister aborted because of a failure/error")
            else:
                self.logger.info("Run completed")
            raise SystemExit(1)

    @catch_system_exit_exception
    def run(self) -> int:
        """Run twister."""
        self.start_time = time.time()

        self.configure_color_output()

        previous_results = self.clean_previous_results()
        previous_results_file = self.export_previous_results(previous_results)

        setup_logging(
            self.options.outdir,
            self.options.log_file,
            self.options.log_level,
            self.options.timestamps,
        )

        self.env = TwisterEnv(self.options, self.default_options)
        self.env.discover()

        self.hwm = self.env.hwm = self.discover_hardware_map(self.env)

        self.tplan = self.create_test_plan(self.env)

        # if we are using command line platform filter, no need to list every
        # other platform as excluded, we know that already.
        # Show only the discards that apply to the selected platforms on the
        # command line

        if self.options.verbose > 0:
            self.verbose(self.tplan)

        self.report = self.create_report(self.tplan, self.env)

        self.match_platforms_names(self.hwm, self.tplan)

        if self.options.device_testing and not self.options.build_only:
            print("\nDevice testing on:")
            self.hwm.dump(filtered=self.tplan.selected_platforms)
            print("")

        if self.options.dry_run:
            duration = time.time() - self.start_time
            self.logger.info(f"Completed in {duration:.2f} seconds")
            return 0

        if self.options.short_build_path:
            self.tplan.create_build_dir_links()

        self.runner = self.execute(self.tplan, self.env, self.hwm)

        self.prepare_reports(self.report, previous_results_file)

        self.logger.info("Run completed")
        return 0


def twister(options: argparse.Namespace, default_options: argparse.Namespace) -> int:
    """Run twister."""
    # function for backward compatibility
    warnings.warn(
        "Function `twister` is deprecated, use `Twister` class instead",
        DeprecationWarning,
        stacklevel=2
    )
    return Twister(options, default_options).run()


def main(argv: Sequence[str] | None = None) -> int:
    """Main function to run twister."""
    try:
        python_version_guard()
        twister_instance = Twister.create_instance(argv)
        return twister_instance.run()
    finally:
        close_logging()
        if (os.name != "nt") and os.isatty(1):
            # (OS is not Windows) and (stdout is interactive)
            os.system("stty sane <&1")
