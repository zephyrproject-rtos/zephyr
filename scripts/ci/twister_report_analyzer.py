#!/usr/bin/env python3
# Copyright (c) 2025 Nordic Semiconductor NA
# SPDX-License-Identifier: Apache-2.0
from __future__ import annotations

import argparse
import csv
import json
import logging
import os
import textwrap
from dataclasses import asdict, dataclass, field, is_dataclass

logger = logging.getLogger(__name__)


def create_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
        description='Analyzes Twister JSON reports',
        epilog=(
            textwrap.dedent("""
            Example usage:
                To analyze errors with predefined CMake and Build error patterns, run:
                > python %(prog)s twister_reports/*.json --long-summary
                The summary will be saved to twister_report_summary.json file unless --output option is used.
                To save error summary to CSV file, use --output-csv option (number of test files is limited to 100):
                > python %(prog)s twister_reports/*.json --output-csv twister_report_summary.csv
                One can use --error-patterns option to provide custom error patterns file:
                > python %(prog)s **/twister.json --error-patterns error_patterns.txt
        """)  # noqa E501
        ),
    )
    parser.add_argument('inputs', type=str, nargs="+", help='twister.json files to read')
    parser.add_argument(
        '--error-patterns',
        type=str,
        help='text file with custom error patterns, ' 'each entry must be separated by newlines',
    )
    parser.add_argument(
        '--output',
        type=str,
        default='twister_report_summary.json',
        help='output json file name, default: twister_report_summary.json',
    )
    parser.add_argument('--output-csv', type=str, help='output csv file name')
    parser.add_argument(
        '--output-md', type=str, help='output markdown file name to store table with errors'
    )
    parser.add_argument(
        '--status',
        action='store_true',
        help='add statuses of testsuites and testcases to the summary',
    )
    parser.add_argument(
        '--platforms',
        action='store_true',
        help='add errors per platform to the summary',
    )
    parser.add_argument(
        '--long-summary',
        action='store_true',
        help='show all matched errors grouped by reason, otherwise show only most common errors',
    )

    parser.add_argument(
        '-ll',
        '--log-level',
        type=str.upper,
        default='INFO',
        choices=['DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL'],
    )
    return parser


@dataclass
class Counters:
    counters: dict[str, TestCollection] = field(default_factory=dict)

    def add_counter(self, key: str, test: str = '') -> None:
        self.counters[key] = self.counters.get(key, TestCollection())
        self.counters[key].append(test)

    def print_counters(self, indent: int = 0):
        for key, value in self.counters.items():
            print(f'{" " * indent}{value.quantity:4}    {key}')
            if value.has_subcounters():
                value.subcounters.print_counters(indent + 4)

    def sort_by_quantity(self):
        self.counters = dict(
            sorted(self.counters.items(), key=lambda item: item[1].quantity, reverse=True)
        )
        for value in self.counters.values():
            if value.has_subcounters():
                value.subcounters.sort_by_quantity()

    def get_next_entry(self, depth: int = 0, max_depth: int = 10):
        for key, value in self.counters.items():
            # limit number of test files to 100 to not exceed CSV cell limit
            yield depth, value.quantity, key, ', '.join(value.tests[0:100])
            if value.has_subcounters() and depth < max_depth:
                yield from value.subcounters.get_next_entry(depth + 1, max_depth)

    def _flatten(self):
        """
        Yield all deepest counters in a flat structure.
        Deepest counters refer to those counters which
        do not contain any further nested subcounters.
        """
        for key, value in self.counters.items():
            if value.has_subcounters():
                yield from value.subcounters._flatten()
            else:
                yield key, value

    def get_most_common(self, n: int = 10):
        return dict(sorted(self._flatten(), key=lambda item: item[1].quantity, reverse=True)[:n])


@dataclass
class TestCollection:
    quantity: int = 0
    tests: list[str] = field(default_factory=list)
    subcounters: Counters = field(default_factory=Counters)

    def append(self, test: str = ''):
        self.quantity += 1
        if test:
            self.tests.append(test)

    def has_subcounters(self):
        return bool(self.subcounters.counters)


class TwisterReports:
    def __init__(self):
        self.status: Counters = Counters()
        self.errors: Counters = Counters()
        self.platforms: Counters = Counters()

    def parse_report(self, json_filename):
        logger.info(f'Process {json_filename}')
        with open(json_filename) as json_results:
            json_data = json.load(json_results)

        for ts in json_data.get('testsuites', []):
            self.parse_statuses(ts)

        for ts in json_data.get('testsuites', []):
            self.parse_testsuite(ts)

    def parse_statuses(self, testsuite):
        ts_status = testsuite.get('status', 'no status in testsuite')
        self.status.add_counter(ts_status)
        # Process testcases
        for tc in testsuite.get('testcases', []):
            tc_status = tc.get('status')
            self.status.counters[ts_status].subcounters.add_counter(tc_status)

    def parse_testsuite(self, testsuite):
        ts_status = testsuite.get('status') or 'no status in testsuite'
        if ts_status not in ('error', 'failed'):
            return

        ts_platform = testsuite.get('platform') or 'Unknown platform'
        self.platforms.add_counter(ts_platform)
        ts_reason = testsuite.get('reason') or 'Unknown reason'
        ts_log = testsuite.get('log')
        test_identifier = f'{testsuite.get("platform")}:{testsuite.get("name")}'

        # CMake and Build failures are treated separately.
        # Extract detailed information to group errors. Keep the parsing methods
        # to allow for further customization and keep backward compatibility.
        if ts_reason.startswith('CMake build failure'):
            reason = 'CMake build failure'
            self.errors.add_counter(reason)
            error_key = ts_reason.split(reason, 1)[-1].lstrip(' -')
            if not error_key:
                error_key = self._parse_cmake_build_failure(ts_log)
            self.errors.counters[reason].subcounters.add_counter(error_key, test_identifier)
            ts_reason = reason
        elif ts_reason.startswith('Build failure'):
            reason = 'Build failure'
            self.errors.add_counter(reason)
            error_key = ts_reason.split(reason, 1)[-1].lstrip(' -')
            if not error_key:
                error_key = self._parse_build_failure(ts_log)
            self.errors.counters[reason].subcounters.add_counter(error_key, test_identifier)
            ts_reason = reason
        else:
            self.errors.add_counter(ts_reason)

        # Process testcases
        for tc in testsuite.get('testcases', []):
            tc_reason = tc.get('reason')
            tc_log = tc.get('log')
            if tc_reason and tc_log:
                self.errors.counters[ts_reason].subcounters.add_counter(tc_reason, test_identifier)

        if not self.errors.counters[ts_reason].has_subcounters():
            self.errors.counters[ts_reason].tests.append(test_identifier)

    def _parse_cmake_build_failure(self, log: str) -> str | None:
        last_warning = 'no warning found'
        lines = log.splitlines()
        for i, line in enumerate(lines):
            if "warning: " in line:
                last_warning = line
            elif "devicetree error: " in line:
                return "devicetree error"
            elif "fatal error: " in line:
                return line[line.index('fatal error: ') :].strip()
            elif "error: " in line:  # error: Aborting due to Kconfig warnings
                if "undefined symbol" in last_warning:
                    return last_warning[last_warning.index('undefined symbol') :].strip()
                return last_warning
            elif "CMake Error at" in line:
                for next_line in lines[i + 1 :]:
                    if next_line.strip():
                        return line + ' ' + next_line
                return line
        return "No matching CMake error pattern found"

    def _parse_build_failure(self, log: str) -> str | None:
        last_warning = ''
        lines = log.splitlines()
        for i, line in enumerate(lines):
            if "undefined reference" in line:
                return line[line.index('undefined reference') :].strip()
            elif "error: ld returned" in line:
                if last_warning:
                    return last_warning
                elif "overflowed by" in lines[i - 1]:
                    return "ld.bfd: region overflowed"
                elif "ld.bfd: warning: " in lines[i - 1]:
                    return "ld.bfd:" + lines[i - 1].split("ld.bfd:", 1)[-1]
                return line
            elif "error: " in line:
                return line[line.index('error: ') :].strip()
            elif ": in function " in line:
                last_warning = line[line.index('in function') :].strip()
        return "No matching build error pattern found"

    def sort_counters(self):
        self.status.sort_by_quantity()
        self.platforms.sort_by_quantity()
        self.errors.sort_by_quantity()


class TwisterReportsWithPatterns(TwisterReports):
    def __init__(self, error_patterns_file):
        super().__init__()
        self.error_patterns = []
        self.add_error_patterns(error_patterns_file)

    def add_error_patterns(self, filename):
        with open(filename) as f:
            self.error_patterns = [
                line
                for line in f.read().splitlines()
                if line.strip() and not line.strip().startswith('#')
            ]
        logger.info(f'Loaded {len(self.error_patterns)} error patterns from {filename}')

    def parse_testsuite(self, testsuite):
        ts_status = testsuite.get('status') or 'no status in testsuite'
        if ts_status not in ('error', 'failed'):
            return

        ts_log = testsuite.get('log')
        test_identifier = f'{testsuite.get("platform")}:{testsuite.get("name")}'
        if key := self._parse_log_with_error_paterns(ts_log):
            self.errors.add_counter(key, test_identifier)
        # Process testcases
        for tc in testsuite.get('testcases', []):
            tc_log = tc.get('log')
            if tc_log and (key := self._parse_log_with_error_paterns(tc_log)):
                self.errors.add_counter(key, test_identifier)

    def _parse_log_with_error_paterns(self, log: str) -> str | None:
        for line in log.splitlines():
            for error_pattern in self.error_patterns:
                if error_pattern in line:
                    logger.debug(f'Matched: {error_pattern} in {line}')
                    return error_pattern
        return None


class EnhancedJSONEncoder(json.JSONEncoder):
    def default(self, o):
        if is_dataclass(o):
            return asdict(o)
        return super().default(o)


def dump_to_json(filename, data):
    with open(filename, 'w') as f:
        json.dump(data, f, indent=4, cls=EnhancedJSONEncoder)
    logger.info(f'Data saved to {filename}')


def dump_to_csv(filename, data: Counters):
    with open(filename, 'w', newline='') as csvfile:
        csvwriter = csv.writer(csvfile)
        # Write headers
        csvwriter.writerow(['Depth', 'Counter', 'Key', 'Tests'])
        # Write error data
        for csv_data in data.get_next_entry():
            csvwriter.writerow(csv_data)
    logger.info(f'Data saved to {filename}')


def dump_markdown_table(filename, data: Counters, max_depth=2):
    with open(filename, 'w', newline='') as md:
        for depth, quantity, key, _ in data.get_next_entry(max_depth=max_depth):
            if depth == 0:
                md.write('\n')
            md.write(f'| {quantity:4} | {key} |\n')
            if depth == 0:
                md.write('|-------|------|\n')
    logger.info(f'Markdown table saved to {filename}')


def summary_with_most_common_errors(errors: Counters, limit: int = 15):
    print('\nMost common errors summary:')
    for key, value in errors.get_most_common(n=limit).items():
        print(f'{value.quantity:4}    {key}')


def main():
    parser = create_parser()
    args = parser.parse_args()

    logging.basicConfig(level=args.log_level, format='%(levelname)-8s:  %(message)s')
    logger = logging.getLogger()

    if args.error_patterns:
        reports = TwisterReportsWithPatterns(args.error_patterns)
    else:
        reports = TwisterReports()

    for filename in args.inputs:
        if os.path.exists(filename):
            reports.parse_report(filename)
        else:
            logger.warning(f'File not found: {filename}')

    reports.sort_counters()
    dump_to_json(
        args.output,
        {'status': reports.status, 'platforms': reports.platforms, 'errors': reports.errors},
    )

    if args.status:
        print('\nTestsuites and testcases status summary:')
        reports.status.print_counters()

    if not reports.errors.counters:
        return

    if args.platforms and reports.platforms.counters:
        print('\nErrors per platform:')
        reports.platforms.print_counters()

    if args.long_summary:
        print('\nErrors summary:')
        reports.errors.print_counters()
    else:
        summary_with_most_common_errors(reports.errors)

    if args.output_csv:
        dump_to_csv(args.output_csv, reports.errors)
    if args.output_md:
        dump_markdown_table(args.output_md, reports.errors, max_depth=2)


if __name__ == '__main__':
    main()
