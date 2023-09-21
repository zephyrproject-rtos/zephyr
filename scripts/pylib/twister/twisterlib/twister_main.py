# vim: set syntax=python ts=4 :
#
# Copyright (c) 2022 Google
# SPDX-License-Identifier: Apache-2.0

import colorama
import logging
import os
import shutil
import sys
import time

from colorama import Fore

from twisterlib.testplan import TestPlan
from twisterlib.reports import Reporting
from twisterlib.hardwaremap import HardwareMap
from twisterlib.coverage import run_coverage
from twisterlib.runner import TwisterRunner
from twisterlib.environment import TwisterEnv
from twisterlib.package import Artifacts

logger = logging.getLogger("twister")
logger.setLevel(logging.DEBUG)


def setup_logging(outdir, log_file, verbose, timestamps):
    # create file handler which logs even debug messages
    if log_file:
        fh = logging.FileHandler(log_file)
    else:
        fh = logging.FileHandler(os.path.join(outdir, "twister.log"))

    fh.setLevel(logging.DEBUG)

    # create console handler with a higher log level
    ch = logging.StreamHandler()

    if verbose > 1:
        ch.setLevel(logging.DEBUG)
    else:
        ch.setLevel(logging.INFO)

    # create formatter and add it to the handlers
    if timestamps:
        formatter = logging.Formatter("%(asctime)s - %(levelname)s - %(message)s")
    else:
        formatter = logging.Formatter("%(levelname)-7s - %(message)s")

    formatter_file = logging.Formatter(
        "%(asctime)s - %(name)s - %(levelname)s - %(message)s"
    )
    ch.setFormatter(formatter)
    fh.setFormatter(formatter_file)

    # add the handlers to logger
    logger.addHandler(ch)
    logger.addHandler(fh)


def init_color(colorama_strip):
    colorama.init(strip=colorama_strip)


def main(options):
    start_time = time.time()

    # Configure color output
    color_strip = False if options.force_color else None

    colorama.init(strip=color_strip)
    init_color(colorama_strip=color_strip)

    previous_results = None
    # Cleanup
    if options.no_clean or options.only_failed or options.test_only:
        if os.path.exists(options.outdir):
            print("Keeping artifacts untouched")
    elif options.last_metrics:
        ls = os.path.join(options.outdir, "twister.json")
        if os.path.exists(ls):
            with open(ls, "r") as fp:
                previous_results = fp.read()
        else:
            sys.exit(f"Can't compare metrics with non existing file {ls}")
    elif os.path.exists(options.outdir):
        if options.clobber_output:
            print("Deleting output directory {}".format(options.outdir))
            shutil.rmtree(options.outdir)
        else:
            for i in range(1, 100):
                new_out = options.outdir + ".{}".format(i)
                if not os.path.exists(new_out):
                    print("Renaming output directory to {}".format(new_out))
                    shutil.move(options.outdir, new_out)
                    break

    previous_results_file = None
    os.makedirs(options.outdir, exist_ok=True)
    if options.last_metrics and previous_results:
        previous_results_file = os.path.join(options.outdir, "baseline.json")
        with open(previous_results_file, "w") as fp:
            fp.write(previous_results)

    VERBOSE = options.verbose
    setup_logging(options.outdir, options.log_file, VERBOSE, options.timestamps)

    env = TwisterEnv(options)
    env.discover()

    hwm = HardwareMap(env)
    ret = hwm.discover()
    if ret == 0:
        return 0

    env.hwm = hwm

    tplan = TestPlan(env)
    try:
        tplan.discover()
    except RuntimeError as e:
        logger.error(f"{e}")
        return 1

    if tplan.report() == 0:
        return 0

    try:
        tplan.load()
    except RuntimeError as e:
        logger.error(f"{e}")
        return 1

    if options.list_tests and options.platform:
        tplan.report_platform_tests(options.platform)
        return 0

    if VERBOSE > 1:
        # if we are using command line platform filter, no need to list every
        # other platform as excluded, we know that already.
        # Show only the discards that apply to the selected platforms on the
        # command line

        for i in tplan.instances.values():
            if i.status == "filtered":
                if options.platform and i.platform.name not in options.platform:
                    continue
                logger.debug(
                    "{:<25} {:<50} {}SKIPPED{}: {}".format(
                        i.platform.name,
                        i.testsuite.name,
                        Fore.YELLOW,
                        Fore.RESET,
                        i.reason,
                    )
                )

    if options.report_excluded:
        tplan.report_excluded_tests()
        return 0

    report = Reporting(tplan, env)
    plan_file = os.path.join(options.outdir, "testplan.json")
    if not os.path.exists(plan_file):
        report.json_report(plan_file)

    if options.save_tests:
        report.json_report(options.save_tests)
        return 0

    if options.device_testing and not options.build_only:
        print("\nDevice testing on:")
        hwm.dump(filtered=tplan.selected_platforms)
        print("")

    if options.dry_run:
        duration = time.time() - start_time
        logger.info("Completed in %d seconds" % (duration))
        return 0

    if options.short_build_path:
        tplan.create_build_dir_links()

    runner = TwisterRunner(tplan.instances, tplan.testsuites, env)
    runner.duts = hwm.duts
    runner.run()

    # figure out which report to use for size comparison
    report_to_use = None
    if options.compare_report:
        report_to_use = options.compare_report
    elif options.last_metrics:
        report_to_use = previous_results_file

    report.footprint_reports(
        report_to_use,
        options.show_footprint,
        options.all_deltas,
        options.footprint_threshold,
        options.last_metrics,
    )

    duration = time.time() - start_time

    if VERBOSE > 1:
        runner.results.summary()

    report.summary(runner.results, options.disable_unrecognized_section_test, duration)

    if options.coverage:
        if not options.build_only:
            run_coverage(tplan, options)
        else:
            logger.info("Skipping coverage report generation due to --build-only.")

    if options.device_testing and not options.build_only:
        hwm.summary(tplan.selected_platforms)

    report.save_reports(
        options.report_name,
        options.report_suffix,
        options.report_dir,
        options.no_update,
        options.platform_reports,
    )

    report.synopsis()

    if options.package_artifacts:
        artifacts = Artifacts(env)
        artifacts.package()

    logger.info("Run completed")
    if (
        runner.results.failed
        or runner.results.error
        or (tplan.warnings and options.warnings_as_errors)
    ):
        return 1

    return 0
