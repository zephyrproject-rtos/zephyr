#!/usr/bin/env python3
# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import os
import sys
from pathlib import Path
import json
import logging
import subprocess
import shutil
import re
import argparse

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)

from twisterlib.error import TwisterRuntimeError
from twisterlib.log_helper import log_command

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE")
if not ZEPHYR_BASE:
    sys.exit("$ZEPHYR_BASE environment variable undefined")

# Use this for internal comparisons; that's what canonicalization is
# for. Don't use it when invoking other components of the build system
# to avoid confusing and hard to trace inconsistencies in error messages
# and logs, generated Makefiles, etc. compared to when users invoke these
# components directly.
# Note "normalization" is different from canonicalization, see os.path.
canonical_zephyr_base = os.path.realpath(ZEPHYR_BASE)


def parse_arguments(args):
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.fromfile_prefix_chars = "+"

    case_select = parser.add_argument_group("Test case selection",
                                            """
Artificially long but functional example:
    $ ./scripts/twister -v     \\
      --testsuite-root tests/ztest/base    \\
      --testsuite-root tests/kernel   \\
      --test      tests/ztest/base/testing.ztest.verbose_0  \\
      --test      tests/kernel/fifo/fifo_api/kernel.fifo

   "kernel.fifo.poll" is one of the test section names in
                                 __/fifo_api/testcase.yaml
    """)

    compare_group_option = parser.add_mutually_exclusive_group()

    platform_group_option = parser.add_mutually_exclusive_group()

    run_group_option = parser.add_mutually_exclusive_group()

    device = parser.add_mutually_exclusive_group(required="--device-testing" in sys.argv)

    test_or_build = parser.add_mutually_exclusive_group()

    test_xor_subtest = case_select.add_mutually_exclusive_group()

    test_xor_generator = case_select.add_mutually_exclusive_group()

    valgrind_asan_group = parser.add_mutually_exclusive_group()

    case_select.add_argument(
        "-E",
        "--save-tests",
        metavar="FILENAME",
        action="store",
        help="Append list of tests and platforms to be run to file.")

    case_select.add_argument(
        "-F",
        "--load-tests",
        metavar="FILENAME",
        action="store",
        help="Load list of tests and platforms to be run from file.")

    case_select.add_argument(
        "-T", "--testsuite-root", action="append", default=[],
        help="Base directory to recursively search for test cases. All "
             "testcase.yaml files under here will be processed. May be "
             "called multiple times. Defaults to the 'samples/' and "
             "'tests/' directories at the base of the Zephyr tree.")

    case_select.add_argument(
        "-f",
        "--only-failed",
        action="store_true",
        help="Run only those tests that failed the previous twister run "
             "invocation.")

    case_select.add_argument("--list-tests", action="store_true",
                             help="""List of all sub-test functions recursively found in
        all --testsuite-root arguments. Note different sub-tests can share
        the same section name and come from different directories.
        The output is flattened and reports --sub-test names only,
        not their directories. For instance net.socket.getaddrinfo_ok
        and net.socket.fd_set belong to different directories.
        """)

    case_select.add_argument("--list-test-duplicates", action="store_true",
                             help="""List tests with duplicate identifiers.
        """)

    case_select.add_argument("--test-tree", action="store_true",
                             help="""Output the test plan in a tree form""")

    compare_group_option.add_argument("--compare-report",
                        help="Use this report file for size comparison")

    compare_group_option.add_argument(
        "-m", "--last-metrics", action="store_true",
        help="Compare with the results of the previous twister "
             "invocation")

    platform_group_option.add_argument(
        "-G",
        "--integration",
        action="store_true",
        help="Run integration tests")

    platform_group_option.add_argument(
        "--emulation-only", action="store_true",
        help="Only build and run emulation platforms")

    run_group_option.add_argument(
        "--device-testing", action="store_true",
        help="Test on device directly. Specify the serial device to "
             "use with the --device-serial option.")

    run_group_option.add_argument("--generate-hardware-map",
                        help="""Probe serial devices connected to this platform
                        and create a hardware map file to be used with
                        --device-testing
                        """)

    device.add_argument("--device-serial",
                        help="""Serial device for accessing the board
                        (e.g., /dev/ttyACM0)
                        """)

    device.add_argument("--device-serial-pty",
                        help="""Script for controlling pseudoterminal.
                        Twister believes that it interacts with a terminal
                        when it actually interacts with the script.

                        E.g "twister --device-testing
                        --device-serial-pty <script>
                        """)

    device.add_argument("--hardware-map",
                        help="""Load hardware map from a file. This will be used
                        for testing on hardware that is listed in the file.
                        """)

    test_or_build.add_argument(
        "-b", "--build-only", action="store_true", default="--prep-artifacts-for-testing" in sys.argv,
        help="Only build the code, do not attempt to run the code on targets.")

    test_or_build.add_argument(
        "--prep-artifacts-for-testing", action="store_true",
        help="Generate artifacts for testing, do not attempt to run the"
              "code on targets.")

    test_or_build.add_argument(
        "--test-only", action="store_true",
        help="""Only run device tests with current artifacts, do not build
             the code""")

    test_xor_subtest.add_argument(
        "-s", "--test", action="append",
        help="Run only the specified testsuite scenario. These are named by "
             "<path/relative/to/Zephyr/base/section.name.in.testcase.yaml>")

    test_xor_subtest.add_argument(
        "--sub-test", action="append",
        help="""Recursively find sub-test functions and run the entire
        test section where they were found, including all sibling test
        functions. Sub-tests are named by:
        section.name.in.testcase.yaml.function_name_without_test_prefix
        Example: In kernel.fifo.fifo_loop: 'kernel.fifo' is a section name
        and 'fifo_loop' is a name of a function found in main.c without test prefix.
        """)

    valgrind_asan_group.add_argument(
        "--enable-valgrind", action="store_true",
        help="""Run binary through valgrind and check for several memory access
        errors. Valgrind needs to be installed on the host. This option only
        works with host binaries such as those generated for the native_posix
        configuration and is mutual exclusive with --enable-asan.
        """)

    valgrind_asan_group.add_argument(
        "--enable-asan", action="store_true",
        help="""Enable address sanitizer to check for several memory access
        errors. Libasan needs to be installed on the host. This option only
        works with host binaries such as those generated for the native_posix
        configuration and is mutual exclusive with --enable-valgrind.
        """)

    # Start of individual args place them in alpha-beta order

    board_root_list = ["%s/boards" % ZEPHYR_BASE,
                       "%s/scripts/pylib/twister/boards" % ZEPHYR_BASE]

    parser.add_argument(
        "-A", "--board-root", action="append", default=board_root_list,
        help="""Directory to search for board configuration files. All .yaml
files in the directory will be processed. The directory should have the same
structure in the main Zephyr tree: boards/<arch>/<board_name>/""")

    parser.add_argument(
        "-a", "--arch", action="append",
        help="Arch filter for testing. Takes precedence over --platform. "
             "If unspecified, test all arches. Multiple invocations "
             "are treated as a logical 'or' relationship")

    parser.add_argument(
        "-B", "--subset",
        help="Only run a subset of the tests, 1/4 for running the first 25%%, "
             "3/5 means run the 3rd fifth of the total. "
             "This option is useful when running a large number of tests on "
             "different hosts to speed up execution time.")

    parser.add_argument("-C", "--coverage", action="store_true",
                        help="Generate coverage reports. Implies "
                             "--enable-coverage.")

    parser.add_argument(
        "-c", "--clobber-output", action="store_true",
        help="Cleaning the output directory will simply delete it instead "
             "of the default policy of renaming.")

    parser.add_argument(
        "--cmake-only", action="store_true",
        help="Only run cmake, do not build or run.")

    parser.add_argument("--coverage-basedir", default=ZEPHYR_BASE,
                        help="Base source directory for coverage report.")

    parser.add_argument("--coverage-platform", action="append", default=[],
                        help="Platforms to run coverage reports on. "
                             "This option may be used multiple times. "
                             "Default to what was selected with --platform.")

    parser.add_argument("--coverage-tool", choices=['lcov', 'gcovr'], default='lcov',
                        help="Tool to use to generate coverage report.")

    parser.add_argument("--coverage-formats", action="store", default=None, # default behavior is set in run_coverage
                        help="Output formats to use for generated coverage reports, as a comma-separated list. "
                             "Only used in conjunction with gcovr. "
                             "Default to html. "
                             "Valid options are html, xml, csv, txt, coveralls, sonarqube.")

    parser.add_argument(
        "-D", "--all-deltas", action="store_true",
        help="Show all footprint deltas, positive or negative. Implies "
             "--footprint-threshold=0")

    parser.add_argument(
        "--device-serial-baud", action="store", default=None,
        help="Serial device baud rate (default 115200)")

    parser.add_argument("--disable-asserts", action="store_false",
                        dest="enable_asserts",
                        help="deprecated, left for compatibility")

    parser.add_argument(
        "--disable-unrecognized-section-test", action="store_true",
        default=False,
        help="Skip the 'unrecognized section' test.")

    parser.add_argument(
        "--disable-suite-name-check", action="store_true", default=False,
        help="Disable extended test suite name verification at the beginning "
             "of Ztest test. This option could be useful for tests or "
             "platforms, which from some reasons cannot print early logs.")

    parser.add_argument("-e", "--exclude-tag", action="append",
                        help="Specify tags of tests that should not run. "
                             "Default is to run all tests with all tags.")

    parser.add_argument("--enable-coverage", action="store_true",
                        help="Enable code coverage using gcov.")

    parser.add_argument(
        "--enable-lsan", action="store_true",
        help="""Enable leak sanitizer to check for heap memory leaks.
        Libasan needs to be installed on the host. This option only
        works with host binaries such as those generated for the native_posix
        configuration and when --enable-asan is given.
        """)

    parser.add_argument(
        "--enable-ubsan", action="store_true",
        help="""Enable undefined behavior sanitizer to check for undefined
        behaviour during program execution. It uses an optional runtime library
        to provide better error diagnostics. This option only works with host
        binaries such as those generated for the native_posix configuration.
        """)

    parser.add_argument("--enable-size-report", action="store_true",
                        help="Enable expensive computation of RAM/ROM segment sizes.")

    parser.add_argument(
        "--filter", choices=['buildable', 'runnable'],
        default='buildable',
        help="""Filter tests to be built and executed. By default everything is
        built and if a test is runnable (emulation or a connected device), it
        is run. This option allows for example to only build tests that can
        actually be run. Runnable is a subset of buildable.""")

    parser.add_argument("--force-color", action="store_true",
                        help="Always output ANSI color escape sequences "
                             "even when the output is redirected (not a tty)")

    parser.add_argument("--force-toolchain", action="store_true",
                        help="Do not filter based on toolchain, use the set "
                             " toolchain unconditionally")

    parser.add_argument("--gcov-tool", default=None,
                        help="Path to the gcov tool to use for code coverage "
                             "reports")

    parser.add_argument(
        "-H", "--footprint-threshold", type=float, default=5,
        help="When checking test case footprint sizes, warn the user if "
             "the new app size is greater then the specified percentage "
             "from the last release. Default is 5. 0 to warn on any "
             "increase on app size.")

    parser.add_argument(
        "-i", "--inline-logs", action="store_true",
        help="Upon test failure, print relevant log data to stdout "
             "instead of just a path to it.")

    parser.add_argument(
        "-j", "--jobs", type=int,
        help="Number of jobs for building, defaults to number of CPU threads, "
             "overcommitted by factor 2 when --build-only.")

    parser.add_argument(
        "-K", "--force-platform", action="store_true",
        help="""Force testing on selected platforms,
        even if they are excluded in the test configuration (testcase.yaml)."""
    )

    parser.add_argument(
        "-l", "--all", action="store_true",
        help="Build/test on all platforms. Any --platform arguments "
             "ignored.")

    parser.add_argument("--list-tags", action="store_true",
                        help="List all tags occurring in selected tests.")

    parser.add_argument("--log-file", metavar="FILENAME", action="store",
                        help="Specify a file where to save logs.")

    parser.add_argument(
        "-M", "--runtime-artifact-cleanup", action="store_true",
        help="Delete artifacts of passing tests.")

    test_xor_generator.add_argument(
        "-N", "--ninja", action="store_true",
        default=not any(a in sys.argv for a in ("-k", "--make")),
        help="Use the Ninja generator with CMake. (This is the default)")

    test_xor_generator.add_argument(
        "-k", "--make", action="store_true",
        help="Use the unix Makefile generator with CMake.")

    parser.add_argument(
        "-n", "--no-clean", action="store_true",
        help="Re-use the outdir before building. Will result in "
             "faster compilation since builds will be incremental.")

    # To be removed in favor of --detailed-skipped-report
    parser.add_argument(
        "--no-skipped-report", action="store_true",
        help="""Do not report skipped test cases in junit output. [Experimental]
        """)

    parser.add_argument(
        "--detailed-skipped-report", action="store_true",
        help="Generate a detailed report with all skipped test cases"
             "including those that are filtered based on testsuite definition."
        )

    parser.add_argument(
        "-O", "--outdir",
        default=os.path.join(os.getcwd(), "twister-out"),
        help="Output directory for logs and binaries. "
             "Default is 'twister-out' in the current directory. "
             "This directory will be cleaned unless '--no-clean' is set. "
             "The '--clobber-output' option controls what cleaning does.")

    parser.add_argument(
        "-o", "--report-dir",
        help="""Output reports containing results of the test run into the
        specified directory.
        The output will be both in JSON and JUNIT format
        (twister.json and twister.xml).
        """)

    parser.add_argument("--overflow-as-errors", action="store_true",
                        help="Treat RAM/SRAM overflows as errors.")


    parser.add_argument("-P", "--exclude-platform", action="append", default=[],
            help="""Exclude platforms and do not build or run any tests
            on those platforms. This option can be called multiple times.
            """
            )

    parser.add_argument("--persistent-hardware-map", action='store_true',
                        help="""With --generate-hardware-map, tries to use
                        persistent names for serial devices on platforms
                        that support this feature (currently only Linux).
                        """)

    parser.add_argument(
        "-p", "--platform", action="append",
        help="Platform filter for testing. This option may be used multiple "
             "times. Test suites will only be built/run on the platforms "
             "specified. If this option is not used, then platforms marked "
             "as default in the platform metadata file will be chosen "
             "to build and test. ")

    parser.add_argument(
        "--platform-reports", action="store_true",
        help="""Create individual reports for each platform.
        """)

    parser.add_argument("--pre-script",
                        help="""specify a pre script. This will be executed
                        before device handler open serial port and invoke runner.
                        """)

    parser.add_argument("-Q", "--error-on-deprecations", action="store_false",
                        help="Error on deprecation warnings.")

    parser.add_argument(
        "--quarantine-list",
        metavar="FILENAME",
        help="Load list of test scenarios under quarantine. The entries in "
             "the file need to correspond to the test scenarios names as in "
             "corresponding tests .yaml files. These scenarios "
             "will be skipped with quarantine as the reason.")

    parser.add_argument(
        "--quarantine-verify",
        action="store_true",
        help="Use the list of test scenarios under quarantine and run them"
             "to verify their current status.")

    parser.add_argument("-R", "--enable-asserts", action="store_true",
                        default=True,
                        help="deprecated, left for compatibility")

    parser.add_argument("--report-excluded",
                        action="store_true",
                        help="""List all tests that are never run based on current scope and
            coverage. If you are looking for accurate results, run this with
            --all, but this will take a while...""")

    parser.add_argument(
        "--report-name",
        help="""Create a report with a custom name.
        """)

    parser.add_argument(
        "--report-suffix",
        help="""Add a suffix to all generated file names, for example to add a
        version or a commit ID.
        """)

    parser.add_argument(
        "--retry-failed", type=int, default=0,
        help="Retry failing tests again, up to the number of times specified.")

    parser.add_argument(
        "--retry-interval", type=int, default=60,
        help="Retry failing tests after specified period of time.")

    parser.add_argument(
        "--retry-build-errors", action="store_true",
        help="Retry build errors as well.")

    parser.add_argument(
        "-S", "--enable-slow", action="store_true",
        help="Execute time-consuming test cases that have been marked "
             "as 'slow' in testcase.yaml. Normally these are only built.")

    parser.add_argument(
        "--seed", type=int,
        help="Seed for native posix pseudo-random number generator")

    parser.add_argument(
        "--short-build-path",
        action="store_true",
        help="Create shorter build directory paths based on symbolic links. "
             "The shortened build path will be used by CMake for generating "
             "the build system and executing the build. Use this option if "
             "you experience build failures related to path length, for "
             "example on Windows OS. This option can be used only with "
             "'--ninja' argument (to use Ninja build generator).")

    parser.add_argument(
        "--show-footprint", action="store_true",
        help="Show footprint statistics and deltas since last release."
    )

    parser.add_argument(
        "-t", "--tag", action="append",
        help="Specify tags to restrict which tests to run by tag value. "
             "Default is to not do any tag filtering. Multiple invocations "
             "are treated as a logical 'or' relationship.")

    parser.add_argument("--timestamps",
                        action="store_true",
                        help="Print all messages with time stamps.")

    parser.add_argument(
        "-u",
        "--no-update",
        action="store_true",
         help="Do not update the results of the last run of twister.")

    parser.add_argument(
        "-v",
        "--verbose",
        action="count",
        default=0,
        help="Emit debugging information, call multiple times to increase "
             "verbosity.")

    parser.add_argument("-W", "--disable-warnings-as-errors", action="store_true",
                        help="Do not treat warning conditions as errors.")

    parser.add_argument(
        "--west-flash", nargs='?', const=[],
        help="""Uses west instead of ninja or make to flash when running with
             --device-testing. Supports comma-separated argument list.

        E.g "twister --device-testing --device-serial /dev/ttyACM0
                         --west-flash="--board-id=foobar,--erase"
        will translate to "west flash -- --board-id=foobar --erase"

        NOTE: device-testing must be enabled to use this option.
        """
    )
    parser.add_argument(
        "--west-runner",
        help="""Uses the specified west runner instead of default when running
             with --west-flash.

        E.g "twister --device-testing --device-serial /dev/ttyACM0
                         --west-flash --west-runner=pyocd"
        will translate to "west flash --runner pyocd"

        NOTE: west-flash must be enabled to use this option.
        """
    )

    parser.add_argument(
        "-X", "--fixture", action="append", default=[],
        help="Specify a fixture that a board might support.")

    parser.add_argument(
        "-x", "--extra-args", action="append", default=[],
        help="""Extra CMake cache entries to define when building test cases.
        May be called multiple times. The key-value entries will be
        prefixed with -D before being passed to CMake.
        E.g
        "twister -x=USE_CCACHE=0"
        will translate to
        "cmake -DUSE_CCACHE=0"
        which will ultimately disable ccache.
        """
    )

    parser.add_argument(
        "-y", "--dry-run", action="store_true",
        help="""Create the filtered list of test cases, but don't actually
        run them. Useful if you're just interested in the test plan
        generated for every run and saved in the specified output
        directory (testplan.json).
        """)

    parser.add_argument(
        "-z", "--size", action="append",
        help="Don't run twister. Instead, produce a report to "
             "stdout detailing RAM/ROM sizes on the specified filenames. "
             "All other command line arguments ignored.")

    options = parser.parse_args(args)

    # Very early error handling
    if options.short_build_path and not options.ninja:
        logger.error("--short-build-path requires Ninja to be enabled")
        sys.exit(1)

    if options.device_serial_pty and os.name == "nt":  # OS is Windows
        logger.error("--device-serial-pty is not supported on Windows OS")
        sys.exit(1)

    if options.west_runner and options.west_flash is None:
        logger.error("west-runner requires west-flash to be enabled")
        sys.exit(1)

    if options.west_flash and not options.device_testing:
        logger.error("west-flash requires device-testing to be enabled")
        sys.exit(1)

    if not options.testsuite_root:
        options.testsuite_root = [os.path.join(ZEPHYR_BASE, "tests"),
                                 os.path.join(ZEPHYR_BASE, "samples")]

    if options.show_footprint or options.compare_report:
        options.enable_size_report = True

    if options.coverage:
        options.enable_coverage = True

    if not options.coverage_platform:
        options.coverage_platform = options.platform

    if options.enable_valgrind and not shutil.which("valgrind"):
        logger.error("valgrind enabled but valgrind executable not found")
        sys.exit(1)

    if options.device_testing and (options.device_serial or options.device_serial_pty) and len(options.platform) > 1:
        logger.error("""When --device-testing is used with
                        --device-serial or --device-serial-pty,
                        only one platform is allowed""")
        sys.exit(1)

    if options.coverage_formats and (options.coverage_tool != "gcovr"):
        logger.error("""--coverage-formats can only be used when coverage
                        tool is set to gcovr""")
        sys.exit(1)

    if options.size:
        from twisterlib.size_calc import SizeCalculator
        for fn in options.size:
            sc = SizeCalculator(fn, [])
            sc.size_report()
        sys.exit(1)

    return options


class TwisterEnv:

    def __init__(self, options=None) -> None:
        self.version = None
        self.toolchain = None
        self.options = options
        if options and options.ninja:
            self.generator_cmd = "ninja"
            self.generator = "Ninja"
        else:
            self.generator_cmd = "make"
            self.generator = "Unix Makefiles"
        logger.info(f"Using {self.generator}..")

        if options:
            self.test_roots = options.testsuite_root
        else:
            self.test_roots = None
        if options:
            if not isinstance(options.board_root, list):
                self.board_roots = [self.options.board_root]
            else:
                self.board_roots = self.options.board_root
            self.outdir = os.path.abspath(options.outdir)
        else:
            self.board_roots = None
            self.outdir = None

        self.hwm = None

    def discover(self):
        self.check_zephyr_version()
        self.get_toolchain()

    def check_zephyr_version(self):
        try:
            subproc = subprocess.run(["git", "describe", "--abbrev=12", "--always"],
                                     stdout=subprocess.PIPE,
                                     universal_newlines=True,
                                     cwd=ZEPHYR_BASE)
            if subproc.returncode == 0:
                self.version = subproc.stdout.strip()
                logger.info(f"Zephyr version: {self.version}")
        except OSError:
            logger.info("Cannot read zephyr version.")


    @staticmethod
    def run_cmake_script(args=[]):
        script = os.fspath(args[0])

        logger.debug("Running cmake script %s", script)

        cmake_args = ["-D{}".format(a.replace('"', '')) for a in args[1:]]
        cmake_args.extend(['-P', script])

        cmake = shutil.which('cmake')
        if not cmake:
            msg = "Unable to find `cmake` in path"
            logger.error(msg)
            raise Exception(msg)
        cmd = [cmake] + cmake_args
        log_command(logger, "Calling cmake", cmd)

        kwargs = dict()
        kwargs['stdout'] = subprocess.PIPE
        # CMake sends the output of message() to stderr unless it's STATUS
        kwargs['stderr'] = subprocess.STDOUT

        p = subprocess.Popen(cmd, **kwargs)
        out, _ = p.communicate()

        # It might happen that the environment adds ANSI escape codes like \x1b[0m,
        # for instance if twister is executed from inside a makefile. In such a
        # scenario it is then necessary to remove them, as otherwise the JSON decoding
        # will fail.
        ansi_escape = re.compile(r'\x1B(?:[@-Z\\-_]|\[[0-?]*[ -/]*[@-~])')
        out = ansi_escape.sub('', out.decode())

        if p.returncode == 0:
            msg = "Finished running  %s" % (args[0])
            logger.debug(msg)
            results = {"returncode": p.returncode, "msg": msg, "stdout": out}

        else:
            logger.error("Cmake script failure: %s" % (args[0]))
            results = {"returncode": p.returncode, "returnmsg": out}

        return results

    def get_toolchain(self):
        toolchain_script = Path(ZEPHYR_BASE) / Path('cmake/verify-toolchain.cmake')
        result = self.run_cmake_script([toolchain_script, "FORMAT=json"])

        try:
            if result['returncode']:
                raise TwisterRuntimeError(f"E: {result['returnmsg']}")
        except Exception as e:
            print(str(e))
            sys.exit(2)
        self.toolchain = json.loads(result['stdout'])['ZEPHYR_TOOLCHAIN_VARIANT']
        logger.info(f"Using '{self.toolchain}' toolchain.")
