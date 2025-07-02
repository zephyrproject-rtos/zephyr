#!/usr/bin/env python3
#
# Copyright (c) 2024 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
This script converts memory footprint data prepared by `./footprint/scripts/track.py`
into a JSON files compatible with Twister report schema making them ready for upload
to the same ElasticSearch data storage together with other Twister reports
for analysis, visualization, etc.

The memory footprint input data files (rom.json, ram.json) are expected in directories
sturctured as 'ZEPHYR_VERSION/APPLICATION/FEATURE/BOARD' under the input path(s).
The BOARD name itself can be in HWMv2 format as 'BOARD/SOC' or 'BOARD/SOC/VARIANT'
with the corresponding sub-directories.

For example, an input path `./**/*v3.6.0-rc3-*/footprints/**/frdm_k64f/` will be
expanded by bash to all sub-directories with the 'footprints' data `v3.6.0-rc3`
release commits collected for `frdm_k64f` board.
Note: for the above example to work the bash recursive globbing should be active:
`shopt -s globstar`.

The output `twister_footprint.json` files will be placed into the same directories
as the corresponding input files.

In Twister report a test instance has either long or short name, each needs test
suite name from the test configuration yaml file.
This scripts has `--test-name` parameter to customize how to compose test names
from the plan.txt columns including an additional (last) one whth explicit
test suite name ('dot separated' format).
"""

from __future__ import annotations

from datetime import datetime, timezone
import argparse
import os
import sys
import re
import csv
import logging
import json
from git import Repo
from git.exc import BadName


VERSION_COMMIT_RE = re.compile(r".*-g([a-f0-9]{12})$")
PLAN_HEADERS = ['name', 'feature', 'board', 'application', 'options', 'suite_name']
TESTSUITE_FILENAME = { 'tests': 'testcase.yaml', 'samples': 'sample.yaml' }
FOOTPRINT_FILES = { 'ROM': 'rom.json', 'RAM': 'ram.json' }
RESULT_FILENAME = 'twister_footprint.json'
HWMv2_LEVELS = 3

logger = None
LOG_LEVELS = {
       'DEBUG': (logging.DEBUG, 3),
       'INFO': (logging.INFO, 2),
       'WARNING': (logging.WARNING, 1),
       'ERROR': (logging.ERROR, 0)
     }


def init_logs(logger_name=''):
    global logger

    log_level = os.environ.get('LOG_LEVEL', 'ERROR')
    log_level = LOG_LEVELS[log_level][0] if log_level in LOG_LEVELS else logging.ERROR

    console = logging.StreamHandler(sys.stdout)
    console.setFormatter(logging.Formatter('%(asctime)s - %(levelname)-8s - %(message)s'))

    logger = logging.getLogger(logger_name)
    logger.setLevel(log_level)
    logger.addHandler(console)

def set_verbose(verbose: int):
    levels = { lvl[1]: lvl[0] for lvl in LOG_LEVELS.values() }
    if verbose > len(levels):
        verbose = len(levels)
    if verbose <= 0:
        verbose = 0
    logger.setLevel(levels[verbose])


def parse_args():
    parser = argparse.ArgumentParser(allow_abbrev=False,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=__doc__)

    parser.add_argument('input_paths', metavar='INPUT_PATHS', nargs='+',
        help="Directories with the memory footprint data to convert. "
             "Each directory must have 'ZEPHYR_VERSION/APPLICATION/FEATURE/BOARD' path structure.")

    parser.add_argument('-p', '--plan', metavar='PLAN_FILE_CSV', required=True,
        help="An execution plan (CSV file) with details of what footprint applications "
             "and platforms were chosen to generate the input data. "
             "It is also applied to filter input directories and check their names.")

    parser.add_argument('-o', '--output-fname', metavar='OUTPUT_FNAME', required=False,
        default=RESULT_FILENAME,
        help="Destination JSON file name to create at each of INPUT_PATHS. "
             "Default: '%(default)s'")

    parser.add_argument('-z', '--zephyr_base', metavar='ZEPHYR_BASE', required=False,
        default = os.environ.get('ZEPHYR_BASE'),
        help="Zephyr code base path to use instead of the current ZEPHYR_BASE environment variable. "
             "The script needs Zephyr repository there to read SHA and commit time of builds. "
             "Current default: '%(default)s'")

    parser.add_argument("--test-name",
        choices=['application/suite_name', 'suite_name', 'application', 'name.feature'],
        default='name.feature',
        help="How to compose Twister test instance names using plan.txt columns. "
             "Default: '%(default)s'" )

    parser.add_argument("--no-testsuite-check",
        dest='testsuite_check', action="store_false",
        help="Don't check for applications' testsuite configs in ZEPHYR_BASE.")

    parser.add_argument('-v', '--verbose', required=False, action='count', default=0,
        help="Increase the logging level for each occurrence. Default level: ERROR")

    return parser.parse_args()


def read_plan(fname: str) -> list[dict]:
    plan = []
    with open(fname) as plan_file:
        plan_rows = csv.reader(plan_file)
        plan_vals = [ dict(zip(PLAN_HEADERS, row)) for row in plan_rows ]
        plan = { f"{p['name']}/{p['feature']}/{p['board']}" : p for p in plan_vals }
    return plan


def get_id_from_path(plan, in_path, max_levels=HWMv2_LEVELS):
    data_id = {}
    (in_path, data_id['board']) = os.path.split(in_path)
    if not data_id['board']:
        # trailing '/'
        (in_path, data_id['board']) = os.path.split(in_path)

    for _ in range(max_levels):
        (in_path, data_id['feature']) = os.path.split(in_path)
        (c_head, data_id['app']) = os.path.split(in_path)
        (c_head, data_id['version']) = os.path.split(c_head)
        if not all(data_id.values()):
            # incorrect plan id
            return None
        if f"{data_id['app']}/{data_id['feature']}/{data_id['board']}" in plan:
            return data_id
        else:
            # try with HWMv2 board name one more level deep
            data_id['board'] = f"{data_id['feature']}/{data_id['board']}"

    # not found
    return {}


def main():
    errors = 0
    converted = 0
    skipped = 0
    filtered = 0

    run_date = datetime.now(timezone.utc).isoformat(timespec='seconds')

    init_logs()

    args = parse_args()

    set_verbose(args.verbose)

    if not args.zephyr_base:
        logging.error("ZEPHYR_BASE is not defined.")
        sys.exit(1)

    zephyr_base = os.path.abspath(args.zephyr_base)
    zephyr_base_repo = Repo(zephyr_base)

    logging.info(f"scanning {len(args.input_paths)} directories ...")

    logging.info(f"use plan '{args.plan}'")
    plan = read_plan(args.plan)

    test_name_sep = '/' if '/' in args.test_name else '.'
    test_name_parts = args.test_name.split(test_name_sep)

    for report_path in args.input_paths:
        logging.info(f"convert {report_path}")
        # print(p)
        p_head = os.path.normcase(report_path)
        p_head = os.path.normpath(p_head)
        if not os.path.isdir(p_head):
            logging.error(f"not a directory '{p_head}'")
            errors += 1
            continue

        data_id = get_id_from_path(plan, p_head)
        if data_id is None:
            logging.warning(f"skipped '{report_path}' - not a correct report directory")
            skipped += 1
            continue
        elif not data_id:
            logging.info(f"filtered '{report_path}' - not in the plan")
            filtered += 1
            continue

        r_plan = f"{data_id['app']}/{data_id['feature']}/{data_id['board']}"

        if 'suite_name' in test_name_parts and 'suite_name' not in plan[r_plan]:
            logging.info(f"filtered '{report_path}' - no Twister suite name in the plan.")
            filtered += 1
            continue

        suite_name = test_name_sep.join([plan[r_plan][n] if n in plan[r_plan] else '' for n in test_name_parts])

        # Just some sanity checks of the 'application' in the current ZEPHYR_BASE
        if args.testsuite_check:
            suite_type = plan[r_plan]['application'].split('/')
            if len(suite_type) and suite_type[0] in TESTSUITE_FILENAME:
                suite_conf_name = TESTSUITE_FILENAME[suite_type[0]]
            else:
                logging.error(f"unknown app type to get configuration in '{report_path}'")
                errors += 1
                continue

            suite_conf_fname = os.path.join(zephyr_base, plan[r_plan]['application'], suite_conf_name)
            if not os.path.isfile(suite_conf_fname):
                logging.error(f"test configuration not found for '{report_path}' at '{suite_conf_fname}'")
                errors += 1
                continue


        # Check SHA presence in the current ZEPHYR_BASE
        sha_match = VERSION_COMMIT_RE.search(data_id['version'])
        version_sha = sha_match.group(1) if sha_match else data_id['version']
        try:
            git_commit = zephyr_base_repo.commit(version_sha)
        except BadName:
            logging.error(f"SHA:'{version_sha}' is not found in ZEPHYR_BASE for '{report_path}'")
            errors += 1
            continue


        # Compose twister_footprint.json record - each application (test suite) will have its own
        # simplified header with options, SHA, etc.

        res = {}

        res['environment'] = {
            'zephyr_version': data_id['version'],
            'commit_date':
                git_commit.committed_datetime.astimezone(timezone.utc).isoformat(timespec='seconds'),
            'run_date': run_date,
            'options': {
                'testsuite_root': [ plan[r_plan]['application'] ],
                'build_only': True,
                'create_rom_ram_report': True,
                'footprint_report': 'all',
                'platform': [ plan[r_plan]['board'] ]
            }
        }

        test_suite = {
            'name': suite_name,
            'arch': None,
            'platform': plan[r_plan]['board'],
            'status': 'passed',
            'footprint': {}
        }

        for k,v in FOOTPRINT_FILES.items():
            footprint_fname = os.path.join(report_path, v)
            try:
                with open(footprint_fname, "rt") as footprint_json:
                    logger.debug(f"reading {footprint_fname}")
                    test_suite['footprint'][k] = json.load(footprint_json)
            except FileNotFoundError:
                logger.warning(f"{report_path} missing {v}")

        res['testsuites'] = [test_suite]

        report_fname = os.path.join(report_path, args.output_fname)
        with open(report_fname, "wt") as json_file:
            logger.debug(f"writing {report_fname}")
            json.dump(res, json_file, indent=4, separators=(',',':'))

        converted += 1

    logging.info(f'found={len(args.input_paths)}, converted={converted}, '
                 f'skipped={skipped}, filtered={filtered}, errors={errors}')
    sys.exit(errors != 0)


if __name__ == '__main__':
    main()
