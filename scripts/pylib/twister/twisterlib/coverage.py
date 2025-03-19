# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018-2025 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import contextlib
import glob
import logging
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

logger = logging.getLogger('twister')
logger.setLevel(logging.DEBUG)

supported_coverage_formats = {
    "gcovr": ["html", "xml", "csv", "txt", "coveralls", "sonarqube"],
    "lcov":  ["html", "lcov"]
}


class CoverageTool:
    """ Base class for every supported coverage tool
    """

    def __init__(self):
        self.gcov_tool = None
        self.base_dir = None
        self.output_formats = None
        self.coverage_capture = True
        self.coverage_report = True
        self.coverage_per_instance = False
        self.instances = {}

    @staticmethod
    def factory(tool, jobs=None):
        if tool == 'lcov':
            t =  Lcov(jobs)
        elif tool == 'gcovr':
            t =  Gcovr()
        else:
            logger.error(f"Unsupported coverage tool specified: {tool}")
            return None

        logger.debug(f"Select {tool} as the coverage tool...")
        return t

    @staticmethod
    def retrieve_gcov_data(input_file):
        logger.debug(f"Working on {input_file}")
        extracted_coverage_info = {}
        capture_data = False
        capture_complete = False
        with open(input_file) as fp:
            for line in fp.readlines():
                if re.search("GCOV_COVERAGE_DUMP_START", line):
                    capture_data = True
                    capture_complete = False
                    continue
                if re.search("GCOV_COVERAGE_DUMP_END", line):
                    capture_complete = True
                    # Keep searching for additional dumps
                # Loop until the coverage data is found.
                if not capture_data:
                    continue
                if line.startswith("*"):
                    sp = line.split("<")
                    if len(sp) > 1:
                        # Remove the leading delimiter "*"
                        file_name = sp[0][1:]
                        # Remove the trailing new line char
                        hex_dump = sp[1][:-1]
                    else:
                        continue
                else:
                    continue
                if file_name in extracted_coverage_info:
                    extracted_coverage_info[file_name].append(hex_dump)
                else:
                    extracted_coverage_info[file_name] = [hex_dump]
        if not capture_data:
            capture_complete = True
        return {'complete': capture_complete, 'data': extracted_coverage_info}

    def merge_hexdumps(self, hexdumps):
        # Only one hexdump
        if len(hexdumps) == 1:
            return hexdumps[0]

        with tempfile.TemporaryDirectory() as dir:
            # Write each hexdump to a dedicated temporary folder
            dirs = []
            for idx, dump in enumerate(hexdumps):
                subdir = dir + f'/{idx}'
                os.mkdir(subdir)
                dirs.append(subdir)
                with open(f'{subdir}/tmp.gcda', 'wb') as fp:
                    fp.write(bytes.fromhex(dump))

            # Iteratively call gcov-tool (not gcov) to merge the files
            merge_tool = self.gcov_tool + '-tool'
            for d1, d2 in zip(dirs[:-1], dirs[1:], strict=False):
                cmd = [merge_tool, 'merge', d1, d2, '--output', d2]
                subprocess.call(cmd)

            # Read back the final output file
            with open(f'{dirs[-1]}/tmp.gcda', 'rb') as fp:
                return fp.read(-1).hex()

    def create_gcda_files(self, extracted_coverage_info):
        gcda_created = True
        logger.debug(f"Generating {len(extracted_coverage_info)} gcda files")
        for filename, hexdumps in extracted_coverage_info.items():
            # if kobject_hash is given for coverage gcovr fails
            # hence skipping it problem only in gcovr v4.1
            if "kobject_hash" in filename:
                filename = (filename[:-4]) + "gcno"
                with contextlib.suppress(Exception):
                    os.remove(filename)
                continue

            try:
                hexdump_val = self.merge_hexdumps(hexdumps)
                hex_bytes = bytes.fromhex(hexdump_val)
                with open(filename, 'wb') as fp:
                    fp.write(hex_bytes)
            except ValueError:
                logger.exception(f"Unable to convert hex data for file: {filename}")
                gcda_created = False
            except FileNotFoundError:
                logger.exception(f"Unable to create gcda file: {filename}")
                gcda_created = False
        return gcda_created

    def capture_data(self, outdir):
        coverage_completed = True
        for filename in glob.glob(f"{outdir}/**/handler.log", recursive=True):
            gcov_data = self.__class__.retrieve_gcov_data(filename)
            capture_complete = gcov_data['complete']
            extracted_coverage_info = gcov_data['data']
            if capture_complete:
                gcda_created = self.create_gcda_files(extracted_coverage_info)
                if gcda_created:
                    logger.debug(f"Gcov data captured: {filename}")
                else:
                    logger.error(f"Gcov data invalid for: {filename}")
                    coverage_completed = False
            else:
                logger.error(f"Gcov data capture incomplete: {filename}")
                coverage_completed = False
        return coverage_completed

    def generate(self, outdir):
        coverage_completed = self.capture_data(outdir) if self.coverage_capture else True
        if not coverage_completed or not self.coverage_report:
            return coverage_completed, {}
        build_dirs = None
        if not self.coverage_capture and self.coverage_report and self.coverage_per_instance:
            build_dirs = [instance.build_dir for instance in self.instances.values()]
        reports = {}
        with open(os.path.join(outdir, "coverage.log"), "a") as coveragelog:
            ret, reports = self._generate(outdir, coveragelog, build_dirs)
            if ret == 0:
                report_log = {
                    "html": "HTML report generated: {}".format(
                        os.path.join(outdir, "coverage", "index.html")
                    ),
                    "lcov": "LCOV report generated: {}".format(
                        os.path.join(outdir, "coverage.info")
                    ),
                    "xml": "XML report generated: {}".format(
                        os.path.join(outdir, "coverage", "coverage.xml")
                    ),
                    "csv": "CSV report generated: {}".format(
                        os.path.join(outdir, "coverage", "coverage.csv")
                    ),
                    "txt": "TXT report generated: {}".format(
                        os.path.join(outdir, "coverage", "coverage.txt")
                    ),
                    "coveralls": "Coveralls report generated: {}".format(
                        os.path.join(outdir, "coverage", "coverage.coveralls.json")
                    ),
                    "sonarqube": "Sonarqube report generated: {}".format(
                        os.path.join(outdir, "coverage", "coverage.sonarqube.xml")
                    )
                }
                for r in self.output_formats.split(','):
                    logger.info(report_log[r])
            else:
                coverage_completed = False
        logger.debug(f"All coverage data processed: {coverage_completed}")
        return coverage_completed, reports


class Lcov(CoverageTool):

    def __init__(self, jobs=None):
        super().__init__()
        self.ignores = []
        self.ignore_branch_patterns = []
        self.output_formats = "lcov,html"
        self.version = self.get_version()
        self.jobs = jobs

    def get_version(self):
        try:
            result = subprocess.run(
                ['lcov', '--version'],
                capture_output=True,
                text=True,
                check=True
            )
            version_output = result.stdout.strip().replace('lcov: LCOV version ', '')
            return version_output
        except subprocess.CalledProcessError as e:
            logger.error(f"Unable to determine lcov version: {e}")
            sys.exit(1)
        except FileNotFoundError as e:
            logger.error(f"Unable to find lcov tool: {e}")
            sys.exit(1)

    def add_ignore_file(self, pattern):
        self.ignores.append('*' + pattern + '*')

    def add_ignore_directory(self, pattern):
        self.ignores.append('*/' + pattern + '/*')

    def add_ignore_branch_pattern(self, pattern):
        self.ignore_branch_patterns.append(pattern)

    @property
    def is_lcov_v2(self):
        return self.version.startswith("2")

    def run_command(self, cmd, coveragelog):
        if self.is_lcov_v2:
            # The --ignore-errors source option is added for genhtml as well as
            # lcov to avoid it exiting due to
            # samples/application_development/external_lib/
            cmd += [
                "--ignore-errors", "inconsistent,inconsistent",
                "--ignore-errors", "negative,negative",
                "--ignore-errors", "unused,unused",
                "--ignore-errors", "empty,empty",
                "--ignore-errors", "mismatch,mismatch",
            ]

        cmd_str = " ".join(cmd)
        logger.debug(f"Running {cmd_str}...")
        return subprocess.call(cmd, stdout=coveragelog)

    def run_lcov(self, args, coveragelog):
        if self.is_lcov_v2:
            branch_coverage = "branch_coverage=1"
            if self.jobs is None:
                # Default: --parallel=0 will autodetect appropriate parallelism
                parallel = ["--parallel", "0"]
            elif self.jobs == 1:
                # Serial execution requested, don't parallelize at all
                parallel = []
            else:
                parallel = ["--parallel", str(self.jobs)]
        else:
            branch_coverage = "lcov_branch_coverage=1"
            parallel = []

        cmd = [
            "lcov", "--gcov-tool", self.gcov_tool,
            "--rc", branch_coverage,
        ] + parallel + args
        return self.run_command(cmd, coveragelog)


    def _generate(self, outdir, coveragelog, build_dirs=None):
        coveragefile = os.path.join(outdir, "coverage.info")
        ztestfile = os.path.join(outdir, "ztest.info")

        if build_dirs:
            files = []
            for dir_ in build_dirs:
                files_ = [fname for fname in
                            [os.path.join(dir_, "coverage.info"),
                             os.path.join(dir_, "ztest.info")]
                          if os.path.exists(fname)]
                if not files_:
                    logger.debug("Coverage merge no files in: %s", dir_)
                    continue
                files += files_
            logger.debug("Coverage merge %d reports in %s", len(files), outdir)
            cmd = ["--output-file", coveragefile]
            for filename in files:
                cmd.append("--add-tracefile")
                cmd.append(filename)
        else:
            cmd = ["--capture", "--directory", outdir, "--output-file", coveragefile]
            if self.coverage_per_instance and len(self.instances) == 1:
                invalid_chars = re.compile(r"[^A-Za-z0-9_]")
                cmd.append("--test-name")
                cmd.append(invalid_chars.sub("_", next(iter(self.instances))))
        ret = self.run_lcov(cmd, coveragelog)
        if ret:
            logger.error("LCOV capture report stage failed with %s", ret)
            return ret, {}

        # We want to remove tests/* and tests/ztest/test/* but save tests/ztest
        cmd = ["--extract", coveragefile,
               os.path.join(self.base_dir, "tests", "ztest", "*"),
               "--output-file", ztestfile]
        ret = self.run_lcov(cmd, coveragelog)
        if ret:
            logger.error("LCOV extract report stage failed with %s", ret)
            return ret, {}

        files = []
        if os.path.exists(ztestfile) and os.path.getsize(ztestfile) > 0:
            cmd = ["--remove", ztestfile,
                   os.path.join(self.base_dir, "tests/ztest/test/*"),
                   "--output-file", ztestfile]
            ret = self.run_lcov(cmd, coveragelog)
            if ret:
                logger.error("LCOV remove ztest report stage failed with %s", ret)
                return ret, {}

            files = [coveragefile, ztestfile]
        else:
            files = [coveragefile]

        for i in self.ignores:
            cmd = ["--remove", coveragefile, i, "--output-file", coveragefile]
            ret = self.run_lcov(cmd, coveragelog)
            if ret:
                logger.error("LCOV remove ignores report stage failed with %s", ret)
                return ret, {}

        if 'html' not in self.output_formats.split(','):
            return 0, {}

        cmd = ["genhtml", "--legend", "--branch-coverage",
               "--prefix", self.base_dir,
               "-output-directory", os.path.join(outdir, "coverage")]
        if self.coverage_per_instance:
            cmd.append("--show-details")
        cmd += files
        ret = self.run_command(cmd, coveragelog)
        if ret:
            logger.error("LCOV genhtml report stage failed with %s", ret)

        # TODO: Add LCOV summary coverage report.
        return ret, { 'report': coveragefile, 'ztest': ztestfile, 'summary': None }


class Gcovr(CoverageTool):

    def __init__(self):
        super().__init__()
        self.ignores = []
        self.ignore_branch_patterns = []
        self.output_formats = "html"
        self.version = self.get_version()
        # Different ifdef-ed implementations of the same function should not be
        # in conflict treated by GCOVR as separate objects for coverage statistics.
        self.options = ["-v", "--merge-mode-functions=separate"]


    def get_version(self):
        try:
            result = subprocess.run(
                ['gcovr', '--version'],
                capture_output=True,
                text=True,
                check=True
            )
            version_lines = result.stdout.strip().split('\n')
            if version_lines:
                version_output = version_lines[0].replace('gcovr ', '')
                return version_output
        except subprocess.CalledProcessError as e:
            logger.error(f"Unable to determine gcovr version: {e}")
            sys.exit(1)
        except FileNotFoundError as e:
            logger.error(f"Unable to find gcovr tool: {e}")
            sys.exit(1)

    def add_ignore_file(self, pattern):
        self.ignores.append('.*' + pattern + '.*')

    def add_ignore_directory(self, pattern):
        self.ignores.append(".*/" + pattern + '/.*')

    def add_ignore_branch_pattern(self, pattern):
        self.ignore_branch_patterns.append(pattern)

    @staticmethod
    def _interleave_list(prefix, list):
        tuple_list = [(prefix, item) for item in list]
        return [item for sublist in tuple_list for item in sublist]

    @staticmethod
    def _flatten_list(list):
        return [a for b in list for a in b]

    def collect_coverage(self, outdir, coverage_file, ztest_file, coveragelog):
        excludes = Gcovr._interleave_list("-e", self.ignores)
        if len(self.ignore_branch_patterns) > 0:
            # Last pattern overrides previous values, so merge all patterns together
            merged_regex = "|".join([f"({p})" for p in self.ignore_branch_patterns])
            excludes += ["--exclude-branches-by-pattern", merged_regex]

        # We want to remove tests/* and tests/ztest/test/* but save tests/ztest
        cmd = ["gcovr", "-r", self.base_dir,
               "--gcov-ignore-parse-errors=negative_hits.warn_once_per_file",
               "--gcov-executable", self.gcov_tool,
               "-e", "tests/*"]
        cmd += excludes + self.options + ["--json", "-o", coverage_file, outdir]
        cmd_str = " ".join(cmd)
        logger.debug(f"Running: {cmd_str}")
        coveragelog.write(f"Running: {cmd_str}\n")
        coveragelog.flush()
        ret = subprocess.call(cmd, stdout=coveragelog, stderr=coveragelog)
        if ret:
            logger.error(f"GCOVR failed with {ret}")
            return ret, []

        cmd = ["gcovr", "-r", self.base_dir] + self.options
        cmd += ["--gcov-executable", self.gcov_tool,
                "-f", "tests/ztest", "-e", "tests/ztest/test/*",
                "--json", "-o", ztest_file, outdir]
        cmd_str = " ".join(cmd)
        logger.debug(f"Running: {cmd_str}")
        coveragelog.write(f"Running: {cmd_str}\n")
        coveragelog.flush()
        ret = subprocess.call(cmd, stdout=coveragelog, stderr=coveragelog)
        if ret:
            logger.error(f"GCOVR ztest stage failed with {ret}")
            return ret, []

        return ret, [file_ for file_ in [coverage_file, ztest_file]
                     if os.path.exists(file_) and os.path.getsize(file_) > 0]


    def _generate(self, outdir, coveragelog, build_dirs=None):
        coverage_file = os.path.join(outdir, "coverage.json")
        coverage_summary = os.path.join(outdir, "coverage_summary.json")
        ztest_file = os.path.join(outdir, "ztest.json")

        ret = 0
        cmd_ = []
        files = []
        if build_dirs:
            for dir_ in build_dirs:
                files_ = [fname for fname in
                            [os.path.join(dir_, "coverage.json"),
                             os.path.join(dir_, "ztest.json")]
                          if os.path.exists(fname)]
                if not files_:
                    logger.debug(f"Coverage merge no files in: {dir_}")
                    continue
                files += files_
            logger.debug(f"Coverage merge {len(files)} reports in {outdir}")
            ztest_file = None
            cmd_ = ["--json-pretty", "--json", coverage_file]
        else:
            ret, files = self.collect_coverage(outdir, coverage_file, ztest_file, coveragelog)
            logger.debug(f"Coverage collected {len(files)} reports from: {outdir}")

        if not files:
            logger.warning(f"No coverage files to compose report for {outdir}")
            return ret, {}

        subdir = os.path.join(outdir, "coverage")
        os.makedirs(subdir, exist_ok=True)

        tracefiles = self._interleave_list("--add-tracefile", files)

        # Convert command line argument (comma-separated list) to gcovr flags
        report_options = {
            "html": ["--html", os.path.join(subdir, "index.html"), "--html-details"],
            "xml": ["--xml", os.path.join(subdir, "coverage.xml"), "--xml-pretty"],
            "csv": ["--csv", os.path.join(subdir, "coverage.csv")],
            "txt": ["--txt", os.path.join(subdir, "coverage.txt")],
            "coveralls": ["--coveralls", os.path.join(subdir, "coverage.coveralls.json"),
                          "--coveralls-pretty"],
            "sonarqube": ["--sonarqube", os.path.join(subdir, "coverage.sonarqube.xml")]
        }
        gcovr_options = self._flatten_list(
            [report_options[r] for r in self.output_formats.split(',')]
        )

        cmd = ["gcovr", "-r", self.base_dir] + self.options + gcovr_options + tracefiles
        cmd += cmd_
        cmd += ["--json-summary-pretty", "--json-summary", coverage_summary]
        cmd_str = " ".join(cmd)
        logger.debug(f"Running: {cmd_str}")
        coveragelog.write(f"Running: {cmd_str}\n")
        coveragelog.flush()
        ret = subprocess.call(cmd, stdout=coveragelog, stderr=coveragelog)
        if ret:
            logger.error(f"GCOVR merge report stage failed with {ret}")

        return ret, { 'report': coverage_file, 'ztest': ztest_file, 'summary': coverage_summary }


def choose_gcov_tool(options, is_system_gcov):
    gcov_tool = None
    if not options.gcov_tool:
        zephyr_sdk_gcov_tool = os.path.join(
            os.environ.get("ZEPHYR_SDK_INSTALL_DIR", default=""),
            "x86_64-zephyr-elf/bin/x86_64-zephyr-elf-gcov")
        if os.environ.get("ZEPHYR_TOOLCHAIN_VARIANT") == "llvm":
            llvm_path = os.environ.get("LLVM_TOOLCHAIN_PATH")
            if llvm_path is not None:
                llvm_path = os.path.join(llvm_path, "bin")
            llvm_cov = shutil.which("llvm-cov", path=llvm_path)
            llvm_cov_ext = pathlib.Path(llvm_cov).suffix
            gcov_lnk = os.path.join(options.outdir, f"gcov{llvm_cov_ext}")
            try:
                os.symlink(llvm_cov, gcov_lnk)
            except OSError:
                shutil.copy(llvm_cov, gcov_lnk)
            gcov_tool = gcov_lnk
        elif is_system_gcov:
            gcov_tool = "gcov"
        elif os.path.exists(zephyr_sdk_gcov_tool):
            gcov_tool = zephyr_sdk_gcov_tool
        else:
            logger.error(
                "Can't find a suitable gcov tool. Use --gcov-tool or set ZEPHYR_SDK_INSTALL_DIR."
            )
            sys.exit(1)
    else:
        gcov_tool = str(options.gcov_tool)

    return gcov_tool


def run_coverage_tool(options, outdir, is_system_gcov, instances,
                      coverage_capture, coverage_report):
    coverage_tool = CoverageTool.factory(options.coverage_tool, jobs=options.jobs)
    if not coverage_tool:
        return False, {}

    coverage_tool.gcov_tool = str(choose_gcov_tool(options, is_system_gcov))
    logger.debug(f"Using gcov tool: {coverage_tool.gcov_tool}")

    coverage_tool.instances = instances
    coverage_tool.coverage_per_instance = options.coverage_per_instance
    coverage_tool.coverage_capture = coverage_capture
    coverage_tool.coverage_report = coverage_report
    coverage_tool.base_dir = os.path.abspath(options.coverage_basedir)
    # Apply output format default
    if options.coverage_formats is not None:
        coverage_tool.output_formats = options.coverage_formats
    coverage_tool.add_ignore_file('generated')
    coverage_tool.add_ignore_directory('tests')
    coverage_tool.add_ignore_directory('samples')
    # Ignore branch coverage on LOG_* and LOG_HEXDUMP_* macros
    # Branch misses are due to the implementation of Z_LOG2 and cannot be avoided
    coverage_tool.add_ignore_branch_pattern(r"^\s*LOG_(?:HEXDUMP_)?(?:DBG|INF|WRN|ERR)\(.*")
    # Ignore branch coverage on __ASSERT* macros
    # Covering the failing case is not desirable as it will immediately terminate the test.
    coverage_tool.add_ignore_branch_pattern(r"^\s*__ASSERT(?:_EVAL|_NO_MSG|_POST_ACTION)?\(.*")
    return coverage_tool.generate(outdir)


def has_system_gcov(platform):
    return platform and (platform.type in {"native", "unit"})


def run_coverage(options, testplan):
    """ Summary code coverage over the full test plan's scope.
    """
    is_system_gcov = False

    for plat in options.coverage_platform:
        if has_system_gcov(testplan.get_platform(plat)):
            is_system_gcov = True
            break

    return run_coverage_tool(options, options.outdir, is_system_gcov,
                             instances=testplan.instances,
                             coverage_capture=False,
                             coverage_report=True)


def run_coverage_instance(options, instance):
    """ Per-instance code coverage called by ProjectBuilder ('coverage' operation).
    """
    is_system_gcov = has_system_gcov(instance.platform)
    return run_coverage_tool(options, instance.build_dir, is_system_gcov,
                             instances={instance.name: instance},
                             coverage_capture=True,
                             coverage_report=options.coverage_per_instance)
