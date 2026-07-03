# vim: set syntax=python ts=4 :
#
# Copyright (c) 2018-2025 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

import collections
import contextlib
import filecmp
import glob
import json
import logging
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile
import time

logger = logging.getLogger('twister')

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
        self.coverage_per_test = False
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
        extracted_coverage_info = collections.defaultdict(list)
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
                try:
                    hex_bytes = bytes.fromhex(hex_dump)
                    extracted_coverage_info[file_name].append(hex_bytes)
                except ValueError:
                    logger.exception(f"Unable to convert hex data for file: {file_name}")
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
                    fp.write(dump)

            # Iteratively call gcov-tool (not gcov) to merge the files
            merge_tool = self.gcov_tool + '-tool'
            for d1, d2 in zip(dirs[:-1], dirs[1:], strict=False):
                cmd = [merge_tool, 'merge', d1, d2, '--output', d2]
                subprocess.call(cmd)

            # Read back the final output file
            with open(f'{dirs[-1]}/tmp.gcda', 'rb') as fp:
                return fp.read(-1)

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
                with open(filename, 'wb') as fp:
                    fp.write(hexdump_val)
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

    def generate(self, outdir) -> tuple[bool, dict]:
        gcov_process_start = time.time()
        coverage_completed = self.capture_data(outdir) if self.coverage_capture else True
        if not coverage_completed or not self.coverage_report:
            return coverage_completed, {}
        build_dirs = None
        if (not self.coverage_capture and self.coverage_report
                and (self.coverage_per_instance or self.coverage_per_test)):
            # Aggregate by merging the per-instance tracefiles. In per-test mode
            # those already carry every per-test TN record, and no canonical
            # .gcda exist at the top level to capture from (semihosting writes
            # tagged files only).
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
                # Per-instance reports are generated one per test instance and
                # would flood the console, so log them at debug level and keep
                # only the aggregated report at info level.
                per_instance_report = self.coverage_capture and self.coverage_per_instance
                log = logger.debug if per_instance_report else logger.info
                for r in self.output_formats.split(','):
                    log(report_log[r])
            else:
                coverage_completed = False
        gcov_process_duration = time.time() - gcov_process_start
        logger.debug(f"All coverage data processed: {coverage_completed}")
        logger.info(f"Coverage data processed in {gcov_process_duration:.2f} seconds")
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
        # Send lcov/genhtml diagnostics to the coverage log rather than the
        # console, where they would otherwise flood a per-test run.
        return subprocess.call(cmd, stdout=coveragelog, stderr=coveragelog)

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
        # Note: per-test mode does not use --show-details; per-test attribution
        # is provided by test_matrix.json/the dashboard, and --show-details over
        # thousands of tests would dominate the run time.
        if self.coverage_per_instance:
            cmd.append("--show-details")
        cmd += files
        ret = self.run_command(cmd, coveragelog)
        if ret:
            logger.error("LCOV genhtml report stage failed with %s", ret)

        # TODO: Add LCOV summary coverage report.
        return ret, { 'report': coveragefile, 'ztest': ztestfile, 'summary': None }

    def discover_per_test_serial(self, build_dir):
        """Group tagged per-test gcov dumps from serial handler.log files.

        Returns {tag: {canonical_gcda_path: ("bytes", data)}} for every
        'GCOV_COVERAGE_DUMP_START <tag>' block, the console transport used on
        platforms without semihosting.
        """
        tags = collections.defaultdict(dict)
        for log in glob.glob(f"{build_dir}/**/handler.log", recursive=True):
            for tag, files in retrieve_tagged_gcov_data(log).items():
                for filename, hexdumps in files.items():
                    if "kobject_hash" in filename:
                        continue
                    tags[tag][filename] = ("bytes", self.merge_hexdumps(hexdumps))
        return tags

    def capture_per_test(self, build_dir, scenario, coveragelog):
        """Generate one TN-tagged .info file per Ztest test.

        CONFIG_ZTEST_COVERAGE_PER_TEST makes the target write an isolated set
        of tagged gcda dumps per test. With the semihosting transport these
        land on the host as "<canonical>.gcda@@<suite>.<test>"; otherwise they
        are emitted as tagged blocks on the serial console.

        Each test is captured from its own temporary object tree, containing
        only that test's gcda files plus symlinks to the shared gcno files.
        Coverage is extracted by invoking gcov directly (one process per test,
        far cheaper than a full lcov capture); if that fails, it falls back to
        lcov over the same tree.

        Returns the list of generated per-test tracefiles.
        """
        tags = discover_per_test_semihost(build_dir)
        if not tags:
            tags = self.discover_per_test_serial(build_dir)
        if not tags:
            return []

        per_test_dir = os.path.join(build_dir, "coverage", "tests")
        os.makedirs(per_test_dir, exist_ok=True)

        infos = []
        used_names = set()
        for tag in sorted(tags):
            _suite, _, test = tag.rpartition(".")
            name = f"{scenario}.{test}" if test else f"{scenario}.{tag}"
            # Fall back to the fully-qualified tag if two suites share a test
            # name within the same scenario.
            if name in used_names:
                name = f"{scenario}.{tag}"
            used_names.add(name)

            info = os.path.join(per_test_dir, sanitize_coverage_name(name) + ".info")
            with tempfile.TemporaryDirectory() as objdir:
                gcda_paths = self._materialize_test_tree(objdir, build_dir, tags[tag])
                if not self._gcov_capture(objdir, gcda_paths, name, info):
                    cmd = ["--capture", "--directory", objdir, "--test-name",
                           re.sub(r"[^A-Za-z0-9_]", "_", name),
                           "--output-file", info]
                    self.run_lcov(cmd, coveragelog)

            if os.path.exists(info) and os.path.getsize(info) > 0:
                infos.append(info)
            else:
                logger.error("Per-test coverage capture failed for %s", name)

        return infos

    @staticmethod
    def _materialize_test_tree(objdir, build_dir, entries):
        """Populate objdir with a test's gcda plus symlinks to shared gcno.

        Returns the list of materialized gcda paths.
        """
        gcda_paths = []
        for canonical, spec in entries.items():
            rel = os.path.relpath(canonical, build_dir)
            if rel.startswith(".."):
                rel = canonical.lstrip(os.sep)
            dst_gcda = os.path.join(objdir, rel)
            materialize_canonical_gcda(dst_gcda, spec)
            gcda_paths.append(dst_gcda)
            gcno = canonical[:-len(".gcda")] + ".gcno"
            if os.path.exists(gcno):
                with contextlib.suppress(OSError):
                    os.symlink(gcno, dst_gcda[:-len(".gcda")] + ".gcno")
        return gcda_paths

    def _gcov_capture(self, objdir, gcda_paths, name, info):
        """Capture one test's coverage by invoking gcov directly.

        Writes a TN-tagged tracefile to info. Returns True on success, False if
        gcov is unavailable or produced no usable data (so the caller can fall
        back to lcov).
        """
        if not gcda_paths:
            return False
        cmd = [self.gcov_tool, "--stdout", "--json-format",
               "--branch-probabilities", *gcda_paths]
        try:
            proc = subprocess.run(cmd, cwd=objdir, stdout=subprocess.PIPE,
                                  stderr=subprocess.DEVNULL, check=False)
        except OSError:
            return False
        if proc.returncode != 0 or not proc.stdout:
            return False
        try:
            tracefile = gcov_json_to_tracefile(
                proc.stdout.decode("utf-8", "replace"), name)
        except (ValueError, KeyError):
            logger.exception("Failed to parse gcov output for %s", name)
            return False
        if "SF:" not in tracefile:
            return False
        with open(info, "w") as fp:
            fp.write(tracefile)
        return True


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
        if self.version >= "7.0":
            cmd += ["--gcov-object-directory", outdir]
        if self.version >= "8.0":
            cmd += ["--gcov-ignore-parse-errors=suspicious_hits.warn_once_per_file"]
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
        if self.version >= "7.0":
            cmd += ["--gcov-object-directory", outdir]

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

def sanitize_coverage_name(name):
    """Make a coverage test name safe to use as a filename component."""
    return re.sub(r"[^A-Za-z0-9_.]", "_", name)


def discover_per_test_semihost(build_dir):
    """Group tagged per-test gcda dumps written by the semihosting transport.

    Returns {tag: {canonical_gcda_path: ("file", tagged_path)}} for every
    "<canonical>.gcda@@<tag>" file found under build_dir.
    """
    tags = collections.defaultdict(dict)
    for tagged in glob.glob(f"{build_dir}/**/*@@*", recursive=True):
        canonical, sep, tag = tagged.rpartition("@@")
        if not sep or not canonical.endswith(".gcda") or not tag:
            continue
        tags[tag][canonical] = ("file", tagged)
    return tags


def retrieve_tagged_gcov_data(input_file):
    """Parse tagged per-test gcov dumps from a serial handler.log.

    Returns {tag: {filename: [hex_bytes, ...]}} for every
    'GCOV_COVERAGE_DUMP_START <tag>' block. Untagged blocks (produced by a
    plain end-of-run dump) are ignored, as they carry no per-test attribution.
    """
    tagged = collections.defaultdict(lambda: collections.defaultdict(list))
    current = None
    capture = False
    start_re = re.compile(r"GCOV_COVERAGE_DUMP_START\s*(\S+)?")
    with open(input_file) as fp:
        for line in fp:
            match = start_re.search(line)
            if match:
                current = match.group(1)
                capture = current is not None
                continue
            if re.search("GCOV_COVERAGE_DUMP_END", line):
                capture = False
                current = None
                continue
            if not capture:
                continue
            if line.startswith("*"):
                sp = line.split("<")
                if len(sp) > 1:
                    file_name = sp[0][1:]
                    hex_dump = sp[1].rstrip("\n")
                    try:
                        tagged[current][file_name].append(bytes.fromhex(hex_dump))
                    except ValueError:
                        logger.exception(
                            f"Unable to convert hex data for file: {file_name}")
    return tagged


def materialize_canonical_gcda(canonical, spec):
    """Write a per-test gcda to its canonical path, from a file or raw bytes."""
    kind, value = spec
    os.makedirs(os.path.dirname(canonical), exist_ok=True)
    if kind == "file":
        shutil.copyfile(value, canonical)
    else:
        with open(canonical, "wb") as fp:
            fp.write(value)


def _iter_gcov_json(text):
    """Yield each JSON object from a possibly concatenated gcov --stdout stream."""
    decoder = json.JSONDecoder()
    idx, length = 0, len(text)
    while idx < length:
        while idx < length and text[idx].isspace():
            idx += 1
        if idx >= length:
            break
        obj, idx = decoder.raw_decode(text, idx)
        yield obj


def gcov_json_to_tracefile(text, test_name):
    """Convert gcov --json-format output into an lcov tracefile string.

    Preserves line and branch coverage under a single per-test TN record, so
    the result is equivalent to what "lcov --capture --test-name" would produce
    for the same gcda, but without lcov's per-invocation cost.

    The test name is reduced to lcov's accepted character set to avoid lcov's
    "invalid characters" warning when the tracefiles are later merged. Function
    records are intentionally omitted: gcov's per-function execution counts and
    end lines are reported inconsistently across per-test dumps and would make
    the tracefiles fail lcov's consistency checks when merged.
    """
    files = {}
    for obj in _iter_gcov_json(text):
        for entry in obj.get("files", []):
            path = entry["file"]
            rec = files.setdefault(path, {"lines": {}, "branches": {}})
            for line in entry.get("lines", []):
                num, count = line["line_number"], line.get("count", 0)
                rec["lines"][num] = max(rec["lines"].get(num, 0), count)
                branches = line.get("branches", [])
                if branches:
                    counts = rec["branches"].setdefault(num, [])
                    for idx, br in enumerate(branches):
                        val = br.get("count", 0)
                        if idx < len(counts):
                            counts[idx] += val
                        else:
                            counts.append(val)

    out = [f"TN:{re.sub(r'[^A-Za-z0-9_]', '_', test_name)}"]
    for path in sorted(files):
        rec = files[path]
        out.append(f"SF:{path}")
        brf = brh = 0
        for num in sorted(rec["branches"]):
            for idx, count in enumerate(rec["branches"][num]):
                out.append(f"BRDA:{num},0,{idx},{count}")
                brf += 1
                brh += count > 0
        if brf:
            out.append(f"BRF:{brf}")
            out.append(f"BRH:{brh}")
        lf = lh = 0
        for num in sorted(rec["lines"]):
            count = rec["lines"][num]
            out.append(f"DA:{num},{count}")
            lf += 1
            lh += count > 0
        out.append(f"LF:{lf}")
        out.append(f"LH:{lh}")
        out.append("end_of_record")
    return "\n".join(out) + "\n"


# Source paths matching these substrings are omitted from the per-test matrix,
# mirroring the directories excluded from the regular coverage reports.
_MATRIX_IGNORE_SUBSTRINGS = ("/generated/", "/tests/", "/samples/")



def build_test_matrix(info_files, out_json, base_dir=None):
    """Build a test-to-code coverage matrix from TN-tagged lcov tracefiles.

    Produces a JSON document with two views:
      - "by_line": {source: {line: [tests that covered it]}}
      - "by_test": {test: {source: [lines it covered]}}
    """
    by_line = collections.defaultdict(lambda: collections.defaultdict(set))
    by_test = collections.defaultdict(lambda: collections.defaultdict(set))
    for info in info_files:
        test_name = None
        source = None
        ignored = False
        try:
            with open(info) as fp:
                for raw in fp:
                    line = raw.strip()
                    if line.startswith("TN:"):
                        test_name = line[3:] or \
                            os.path.splitext(os.path.basename(info))[0]
                    elif line.startswith("SF:"):
                        source = line[3:]
                        ignored = any(s in source
                                      for s in _MATRIX_IGNORE_SUBSTRINGS)
                        if base_dir:
                            with contextlib.suppress(ValueError):
                                source = os.path.relpath(source, base_dir)
                    elif line.startswith("DA:") and source is not None and not ignored:
                        fields = line[3:].split(",")
                        lineno = int(fields[0])
                        hits = int(fields[1]) if len(fields) > 1 else 0
                        if hits > 0 and test_name:
                            by_line[source][lineno].add(test_name)
                            by_test[test_name][source].add(lineno)
                    elif line == "end_of_record":
                        source = None
                        ignored = False
        except OSError:
            logger.error("Unable to read per-test tracefile: %s", info)

    matrix = {
        "by_line": {
            src: {str(ln): sorted(tests) for ln, tests in sorted(lines.items())}
            for src, lines in sorted(by_line.items())
        },
        "by_test": {
            test: {src: sorted(lns) for src, lns in sorted(srcs.items())}
            for test, srcs in sorted(by_test.items())
        },
    }
    with open(out_json, "w") as fp:
        json.dump(matrix, fp, indent=2, sort_keys=True)
    return matrix


def merge_test_matrices(matrix_files, out_json):
    """Union several per-instance coverage matrices into an aggregate one.

    Merging the (already relative-pathed and filtered) per-instance
    test_matrix.json files avoids re-parsing every per-test tracefile at the
    end of a run, keeping the aggregate step proportional to the number of
    instances rather than the number of tests.
    """
    by_line = collections.defaultdict(lambda: collections.defaultdict(set))
    by_test = collections.defaultdict(lambda: collections.defaultdict(set))
    for matrix_file in matrix_files:
        try:
            with open(matrix_file) as fp:
                matrix = json.load(fp)
        except (OSError, ValueError):
            logger.error("Unable to read matrix: %s", matrix_file)
            continue
        for src, lines in matrix.get("by_line", {}).items():
            for lineno, tests in lines.items():
                by_line[src][int(lineno)].update(tests)
        for test, srcs in matrix.get("by_test", {}).items():
            for src, lns in srcs.items():
                by_test[test][src].update(lns)

    merged = {
        "by_line": {
            src: {str(ln): sorted(tests) for ln, tests in sorted(lines.items())}
            for src, lines in sorted(by_line.items())
        },
        "by_test": {
            test: {src: sorted(lns) for src, lns in sorted(srcs.items())}
            for test, srcs in sorted(by_test.items())
        },
    }
    with open(out_json, "w") as fp:
        json.dump(merged, fp, indent=2, sort_keys=True)
    return merged


def write_union_tracefile(info_files, out_info):
    """Merge per-test tracefiles into a single, untagged union tracefile.

    Per-test attribution is carried by the coverage matrix, not by lcov, so the
    per-instance report is collapsed to one TN. This keeps the end-of-run
    aggregation (and genhtml) proportional to the number of instances rather
    than the number of individual tests, which is what makes large per-test
    runs tractable.
    """
    files = {}
    for info in info_files:
        source = None
        rec = None
        try:
            with open(info) as fp:
                for raw in fp:
                    line = raw.rstrip("\n")
                    if line.startswith("SF:"):
                        source = line[3:]
                        rec = files.setdefault(source, {"da": {}, "brda": {}})
                    elif rec is None:
                        continue
                    elif line.startswith("DA:"):
                        num, _, rest = line[3:].partition(",")
                        count = int(rest.split(",")[0]) if rest else 0
                        num = int(num)
                        rec["da"][num] = rec["da"].get(num, 0) + count
                    elif line.startswith("BRDA:"):
                        ln, block, branch, taken = line[5:].split(",")
                        key = (int(ln), block, branch)
                        val = 0 if taken == "-" else int(taken)
                        rec["brda"][key] = rec["brda"].get(key, 0) + val
                    elif line == "end_of_record":
                        source = None
                        rec = None
        except OSError:
            logger.error("Unable to read tracefile: %s", info)

    out = []
    for source in sorted(files):
        rec = files[source]
        out.append("TN:")
        out.append(f"SF:{source}")
        brf = brh = 0
        for (ln, block, branch) in sorted(rec["brda"]):
            count = rec["brda"][(ln, block, branch)]
            out.append(f"BRDA:{ln},{block},{branch},{count}")
            brf += 1
            brh += count > 0
        if brf:
            out.append(f"BRF:{brf}")
            out.append(f"BRH:{brh}")
        lf = lh = 0
        for num in sorted(rec["da"]):
            count = rec["da"][num]
            out.append(f"DA:{num},{count}")
            lf += 1
            lh += count > 0
        out.append(f"LF:{lf}")
        out.append(f"LH:{lh}")
        out.append("end_of_record")
    with open(out_info, "w") as fp:
        fp.write("\n".join(out) + "\n")
    return out_info


def run_per_test_coverage(options, instance):
    """Capture per-test coverage for one instance and build its matrix."""
    coverage_tool = CoverageTool.factory(options.coverage_tool, jobs=options.jobs)
    if not isinstance(coverage_tool, Lcov):
        logger.error("Per-test coverage requires the 'lcov' coverage tool.")
        return

    coverage_tool.gcov_tool = str(choose_gcov_tool(
        options, has_system_gcov(instance.platform),
        instances={instance.name: instance}))
    coverage_tool.base_dir = os.path.abspath(options.coverage_basedir)

    scenario = sanitize_coverage_name(instance.testsuite.name)
    build_dir = instance.build_dir
    with open(os.path.join(build_dir, "coverage.log"), "a") as coveragelog:
        infos = coverage_tool.capture_per_test(build_dir, scenario, coveragelog)
        if not infos:
            logger.debug(f"No per-test coverage data for {instance.name}")
            return

    # Collapse the per-test tracefiles into a single untagged instance report.
    # Per-test detail lives in the matrix, so the aggregation stays proportional
    # to the number of instances rather than the number of tests.
    write_union_tracefile(infos, os.path.join(build_dir, "coverage.info"))

    build_test_matrix(
        infos, os.path.join(build_dir, "coverage", "test_matrix.json"),
        base_dir=coverage_tool.base_dir)
    logger.debug(f"Per-test coverage: {len(infos)} tests for {instance.name}")


def try_making_symlink(source: str, link: str):
    """
    Attempts to create a symbolic link from source to link.
    If the link already exists:
    - If it's a symlink pointing to a different source, it's replaced.
    - If it's a regular file with the same content, no action is taken.
    - If it's a regular file with different content, it's replaced with a
    symlink (if possible, otherwise a copy).
    If symlinking fails for any reason (other than the link already existing and
    being correct), it attempts to copy the source to the link.

    Args:
        source (str): The path to the source file.
        link (str): The path where the symbolic link should be created.
    """
    symlink_error = None

    try:
        os.symlink(source, link)
    except FileExistsError:
        if os.path.islink(link):
            if os.readlink(link) == source:
                # Link is already set up
                return
            # Link is pointing to the wrong file, fall below this if/else and
            # it will be replaced
        elif filecmp.cmp(source, link):
            # File contents are the same
            return

        # link exists, but points to a different file. We'll create a new link
        # and replace it atomically with the old one
        temp_filename = f"{link}.{os.urandom(8).hex()}"
        try:
            os.symlink(source, temp_filename)
            os.replace(temp_filename, link)
        except OSError as e:
            symlink_error = e
    except OSError as e:
        symlink_error = e

    if symlink_error:
        logger.error(
            "Error creating symlink: %s, attempting to copy.", str(symlink_error)
        )
        temp_filename = f"{link}.{os.urandom(8).hex()}"
        shutil.copy(source, temp_filename)
        os.replace(temp_filename, link)

def read_cmake_cache_var(build_dir, var):
    """ Return the value of a CMake cache variable from build_dir, or None. """
    cache = os.path.join(build_dir, "CMakeCache.txt")
    # CMake cache entries look like 'NAME:TYPE=VALUE'.
    pattern = re.compile(rf"^{re.escape(var)}:[^=]*=(.*)$")
    try:
        with open(cache) as f:
            for line in f:
                match = pattern.match(line.strip())
                if match:
                    return match.group(1).strip()
    except OSError:
        return None

    return None


def find_cached_path_in_builds(instances, var):
    """ Find a CMake cache path variable across the given instances' builds.

    Each Zephyr build resolves the gcov tool matching its toolchain and
    records it (CMAKE_GCOV) in CMakeCache.txt, so reading it back gives a
    binary that is guaranteed to match the gcno/gcda data. Returns the first
    value that exists on disk, or None.
    """
    for instance in (instances or {}).values():
        value = read_cmake_cache_var(instance.build_dir, var)
        if value and os.path.exists(value):
            return value

    return None


def choose_gcov_tool(options, is_system_gcov, instances=None):
    if options.gcov_tool:
        return str(options.gcov_tool)

    if os.environ.get("ZEPHYR_TOOLCHAIN_VARIANT", "").endswith("/llvm"):
        llvm_path = os.environ.get("LLVM_TOOLCHAIN_PATH")
        if llvm_path is not None:
            llvm_path = os.path.join(llvm_path, "bin")
        llvm_cov = shutil.which("llvm-cov", path=llvm_path)
        llvm_cov_ext = pathlib.Path(llvm_cov).suffix
        gcov_lnk = os.path.join(options.outdir, f"gcov{llvm_cov_ext}")
        try_making_symlink(llvm_cov, gcov_lnk)
        return gcov_lnk

    if is_system_gcov:
        return "gcov"

    # Auto-detect the gcov tool used by the builds. Zephyr's CMake toolchain
    # logic resolves the gcov binary (CMAKE_GCOV) when configuring each build,
    # which for the Zephyr SDK points at the SDK's cross gcov. Preferring it
    # avoids depending on ZEPHYR_SDK_INSTALL_DIR being exported and works for
    # any target architecture.
    gcov_tool = find_cached_path_in_builds(instances, "CMAKE_GCOV")
    if gcov_tool:
        return gcov_tool

    # Fall back to deriving the path from the Zephyr SDK install dir, taken
    # from the environment or, failing that, from what a build recorded.
    sdk_dir = os.environ.get("ZEPHYR_SDK_INSTALL_DIR") or \
        find_cached_path_in_builds(instances, "ZEPHYR_SDK_INSTALL_DIR")
    if sdk_dir:
        zephyr_sdk_gcov_tool = os.path.join(
            sdk_dir, "gnu/x86_64-zephyr-elf/bin/x86_64-zephyr-elf-gcov")
        if os.path.exists(zephyr_sdk_gcov_tool):
            return zephyr_sdk_gcov_tool

    logger.error(
        "Can't find a suitable gcov tool. Use --gcov-tool or set ZEPHYR_SDK_INSTALL_DIR."
    )
    sys.exit(1)


def run_coverage_tool(options, outdir, is_system_gcov, instances,
                      coverage_capture, coverage_report) -> tuple[bool, dict]:
    coverage_tool = CoverageTool.factory(options.coverage_tool, jobs=options.jobs)
    if not coverage_tool:
        return False, {}

    coverage_tool.gcov_tool = str(choose_gcov_tool(options, is_system_gcov, instances))
    logger.debug(f"Using gcov tool: {coverage_tool.gcov_tool}")

    coverage_tool.instances = instances
    coverage_tool.coverage_per_instance = options.coverage_per_instance
    coverage_tool.coverage_per_test = getattr(options, "coverage_per_test", False)
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


def run_coverage(options, testplan) -> tuple[bool, dict]:
    """ Summary code coverage over the full test plan's scope.
    """
    is_system_gcov = False

    for plat in options.coverage_platform:
        if has_system_gcov(testplan.get_platform(plat)):
            is_system_gcov = True
            break

    result = run_coverage_tool(options, options.outdir, is_system_gcov,
                               instances=testplan.instances,
                               coverage_capture=False,
                               coverage_report=True)

    if getattr(options, "coverage_per_test", False):
        matrix_files = [
            os.path.join(instance.build_dir, "coverage", "test_matrix.json")
            for instance in testplan.instances.values()
        ]
        matrix_files = [m for m in matrix_files if os.path.exists(m)]
        if matrix_files:
            matrix_dir = os.path.join(options.outdir, "coverage")
            os.makedirs(matrix_dir, exist_ok=True)
            json_path = os.path.join(matrix_dir, "test_matrix.json")
            merge_test_matrices(matrix_files, json_path)
            logger.info(f"Per-test coverage matrix generated: {json_path}")
            logger.info("Render it with "
                        "scripts/gen_test_matrix_dashboard.py -i "
                        f"{json_path}")

    return result


def run_coverage_instance(options, instance):
    """ Per-instance code coverage called by ProjectBuilder ('coverage' operation).
    """
    is_system_gcov = has_system_gcov(instance.platform)
    result = run_coverage_tool(options, instance.build_dir, is_system_gcov,
                               instances={instance.name: instance},
                               coverage_capture=True,
                               coverage_report=options.coverage_per_instance)

    if getattr(options, "coverage_per_test", False):
        run_per_test_coverage(options, instance)

    return result
