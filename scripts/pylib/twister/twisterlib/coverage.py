# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018-2022 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import sys
import os
import logging
import pathlib
import shutil
import subprocess
import glob
import re
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

    @staticmethod
    def factory(tool, jobs=None):
        if tool == 'lcov':
            t =  Lcov(jobs)
        elif tool == 'gcovr':
            t =  Gcovr()
        else:
            logger.error("Unsupported coverage tool specified: {}".format(tool))
            return None

        logger.debug(f"Select {tool} as the coverage tool...")
        return t

    @staticmethod
    def retrieve_gcov_data(input_file):
        logger.debug("Working on %s" % input_file)
        extracted_coverage_info = {}
        capture_data = False
        capture_complete = False
        with open(input_file, 'r') as fp:
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
            for d1, d2 in zip(dirs[:-1], dirs[1:]):
                cmd = [merge_tool, 'merge', d1, d2, '--output', d2]
                subprocess.call(cmd)

            # Read back the final output file
            with open(f'{dirs[-1]}/tmp.gcda', 'rb') as fp:
                return fp.read(-1).hex()

    def create_gcda_files(self, extracted_coverage_info):
        gcda_created = True
        logger.debug("Generating gcda files")
        for filename, hexdumps in extracted_coverage_info.items():
            # if kobject_hash is given for coverage gcovr fails
            # hence skipping it problem only in gcovr v4.1
            if "kobject_hash" in filename:
                filename = (filename[:-4]) + "gcno"
                try:
                    os.remove(filename)
                except Exception:
                    pass
                continue

            try:
                hexdump_val = self.merge_hexdumps(hexdumps)
                with open(filename, 'wb') as fp:
                    fp.write(bytes.fromhex(hexdump_val))
            except ValueError:
                logger.exception("Unable to convert hex data for file: {}".format(filename))
                gcda_created = False
            except FileNotFoundError:
                logger.exception("Unable to create gcda file: {}".format(filename))
                gcda_created = False
        return gcda_created

    def generate(self, outdir):
        coverage_completed = True
        for filename in glob.glob("%s/**/handler.log" % outdir, recursive=True):
            gcov_data = self.__class__.retrieve_gcov_data(filename)
            capture_complete = gcov_data['complete']
            extracted_coverage_info = gcov_data['data']
            if capture_complete:
                gcda_created = self.create_gcda_files(extracted_coverage_info)
                if gcda_created:
                    logger.debug("Gcov data captured: {}".format(filename))
                else:
                    logger.error("Gcov data invalid for: {}".format(filename))
                    coverage_completed = False
            else:
                logger.error("Gcov data capture incomplete: {}".format(filename))
                coverage_completed = False

        with open(os.path.join(outdir, "coverage.log"), "a") as coveragelog:
            ret = self._generate(outdir, coveragelog)
            if ret == 0:
                report_log = {
                    "html": "HTML report generated: {}".format(os.path.join(outdir, "coverage", "index.html")),
                    "lcov": "LCOV report generated: {}".format(os.path.join(outdir, "coverage.info")),
                    "xml": "XML report generated: {}".format(os.path.join(outdir, "coverage", "coverage.xml")),
                    "csv": "CSV report generated: {}".format(os.path.join(outdir, "coverage", "coverage.csv")),
                    "txt": "TXT report generated: {}".format(os.path.join(outdir, "coverage", "coverage.txt")),
                    "coveralls": "Coveralls report generated: {}".format(os.path.join(outdir, "coverage", "coverage.coveralls.json")),
                    "sonarqube": "Sonarqube report generated: {}".format(os.path.join(outdir, "coverage", "coverage.sonarqube.xml"))
                }
                for r in self.output_formats.split(','):
                    logger.info(report_log[r])
            else:
                coverage_completed = False
        logger.debug("All coverage data processed: {}".format(coverage_completed))
        return coverage_completed


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
            result = subprocess.run(['lcov', '--version'],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE,
                                    text=True, check=True)
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

        excludes = []
        for ignore in self.ignores:
            excludes += ["--exclude", ignore]

        cmd = [
            "lcov", "--gcov-tool", self.gcov_tool,
            "--rc", branch_coverage,
        ] + parallel + args + excludes
        return self.run_command(cmd, coveragelog)

    def _generate(self, outdir, coveragelog):
        coveragefile = os.path.join(outdir, "coverage.info")
        ztestfile = os.path.join(outdir, "ztest.info")

        cmd = ["--capture", "--directory", outdir, "--output-file", coveragefile]
        self.run_lcov(cmd, coveragelog)

        # We want to remove tests/* and tests/ztest/test/* but save tests/ztest
        cmd = ["--extract", coveragefile,
               os.path.join(self.base_dir, "tests", "ztest", "*"),
               "--output-file", ztestfile]
        self.run_lcov(cmd, coveragelog)

        if os.path.exists(ztestfile) and os.path.getsize(ztestfile) > 0:
            cmd = ["--remove", ztestfile,
                   os.path.join(self.base_dir, "tests/ztest/test/*"),
                   "--output-file", ztestfile]
            self.run_lcov(cmd, coveragelog)

            files = [coveragefile, ztestfile]
        else:
            files = [coveragefile]

        for i in self.ignores:
            cmd = ["--remove", coveragefile, i, "--output-file", coveragefile]
            self.run_lcov(cmd, coveragelog)

        if 'html' not in self.output_formats.split(','):
            return 0

        cmd = ["genhtml", "--legend", "--branch-coverage",
               "--prefix", self.base_dir,
               "-output-directory", os.path.join(outdir, "coverage")] + files
        return self.run_command(cmd, coveragelog)


class Gcovr(CoverageTool):

    def __init__(self):
        super().__init__()
        self.ignores = []
        self.ignore_branch_patterns = []
        self.output_formats = "html"
        self.version = self.get_version()

    def get_version(self):
        try:
            result = subprocess.run(['gcovr', '--version'],
                                    stdout=subprocess.PIPE,
                                    stderr=subprocess.PIPE,
                                    text=True, check=True)
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

    def _generate(self, outdir, coveragelog):
        coveragefile = os.path.join(outdir, "coverage.json")
        ztestfile = os.path.join(outdir, "ztest.json")

        excludes = Gcovr._interleave_list("-e", self.ignores)
        if len(self.ignore_branch_patterns) > 0:
            # Last pattern overrides previous values, so merge all patterns together
            merged_regex = "|".join([f"({p})" for p in self.ignore_branch_patterns])
            excludes += ["--exclude-branches-by-pattern", merged_regex]

        # Different ifdef-ed implementations of the same function should not be
        # in conflict treated by GCOVR as separate objects for coverage statistics.
        mode_options = ["--merge-mode-functions=separate"]

        # We want to remove tests/* and tests/ztest/test/* but save tests/ztest
        cmd = ["gcovr", "-r", self.base_dir,
               "--gcov-ignore-parse-errors=negative_hits.warn_once_per_file",
               "--gcov-executable", self.gcov_tool,
               "-e", "tests/*"]
        cmd += excludes + mode_options + ["--json", "-o", coveragefile, outdir]
        cmd_str = " ".join(cmd)
        logger.debug(f"Running {cmd_str}...")
        subprocess.call(cmd, stdout=coveragelog)

        subprocess.call(["gcovr", "-r", self.base_dir, "--gcov-executable",
                         self.gcov_tool, "-f", "tests/ztest", "-e",
                         "tests/ztest/test/*", "--json", "-o", ztestfile,
                         outdir] + mode_options, stdout=coveragelog)

        if os.path.exists(ztestfile) and os.path.getsize(ztestfile) > 0:
            files = [coveragefile, ztestfile]
        else:
            files = [coveragefile]

        subdir = os.path.join(outdir, "coverage")
        os.makedirs(subdir, exist_ok=True)

        tracefiles = self._interleave_list("--add-tracefile", files)

        # Convert command line argument (comma-separated list) to gcovr flags
        report_options = {
            "html": ["--html", os.path.join(subdir, "index.html"), "--html-details"],
            "xml": ["--xml", os.path.join(subdir, "coverage.xml"), "--xml-pretty"],
            "csv": ["--csv", os.path.join(subdir, "coverage.csv")],
            "txt": ["--txt", os.path.join(subdir, "coverage.txt")],
            "coveralls": ["--coveralls", os.path.join(subdir, "coverage.coveralls.json"), "--coveralls-pretty"],
            "sonarqube": ["--sonarqube", os.path.join(subdir, "coverage.sonarqube.xml")]
        }
        gcovr_options = self._flatten_list([report_options[r] for r in self.output_formats.split(',')])

        return subprocess.call(["gcovr", "-r", self.base_dir] + mode_options + gcovr_options + tracefiles,
                               stdout=coveragelog)



def run_coverage(testplan, options):
    use_system_gcov = False
    gcov_tool = None

    for plat in options.coverage_platform:
        _plat = testplan.get_platform(plat)
        if _plat and (_plat.type in {"native", "unit"}):
            use_system_gcov = True
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
        elif use_system_gcov:
            gcov_tool = "gcov"
        elif os.path.exists(zephyr_sdk_gcov_tool):
            gcov_tool = zephyr_sdk_gcov_tool
        else:
            logger.error(f"Can't find a suitable gcov tool. Use --gcov-tool or set ZEPHYR_SDK_INSTALL_DIR.")
            sys.exit(1)
    else:
        gcov_tool = str(options.gcov_tool)

    logger.info("Generating coverage files...")
    logger.info(f"Using gcov tool: {gcov_tool}")
    coverage_tool = CoverageTool.factory(options.coverage_tool, jobs=options.jobs)
    coverage_tool.gcov_tool = gcov_tool
    coverage_tool.base_dir = os.path.abspath(options.coverage_basedir)
    # Apply output format default
    if options.coverage_formats is not None:
        coverage_tool.output_formats = options.coverage_formats
    coverage_tool.add_ignore_file('generated')
    coverage_tool.add_ignore_directory('tests')
    coverage_tool.add_ignore_directory('samples')
    for exclude in options.coverage_exclude:
        coverage_tool.add_ignore_directory(exclude)
    # Ignore branch coverage on LOG_* and LOG_HEXDUMP_* macros
    # Branch misses are due to the implementation of Z_LOG2 and cannot be avoided
    coverage_tool.add_ignore_branch_pattern(r"^\s*LOG_(?:HEXDUMP_)?(?:DBG|INF|WRN|ERR)\(.*")
    # Ignore branch coverage on __ASSERT* macros
    # Covering the failing case is not desirable as it will immediately terminate the test.
    coverage_tool.add_ignore_branch_pattern(r"^\s*__ASSERT(?:_EVAL|_NO_MSG|_POST_ACTION)?\(.*")
    coverage_completed = coverage_tool.generate(options.outdir)
    return coverage_completed
