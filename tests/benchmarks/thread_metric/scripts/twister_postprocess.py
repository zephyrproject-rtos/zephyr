#!/usr/bin/env python3

# Copyright (c) 2025 Intel Corporation
# SPDX-License-Identifier: Apache-2.0

"""
This script post-process Twister report (`twister.json`) after the Thread-Metric
benchmark execution: each test suite of tests/benchmarks/thread_metric
(benchmark.thread_metric.*) will have additional recording properties:

  `benchmark_name` - the corresponding Thread-Metric benchmark name.
  `baseline_median` - baseline metric median used to normalize other values.
  `total_time_period_median` - current metric's median value.
  `total_time_period_normalized` - current metric's values normalized - divided
                                   by the baseline median value.

These additional properties are to simplify further data analysis, for example
to visualize the thread_metrics data collected on ElasticSearch Kibana dashboards
using standard widgets.
"""

import argparse
import json
import os
import statistics
import sys

has_errors = 0
threadx_benchmark_name_prefix = "benchmark.thread_metric"
threadx_benchmark_name = {
    "basic": "Basic Processing (baseline)",
    "cooperative": "Cooperative Scheduling",
    "preemptive": "Preemptive Scheduling",
    "interrupt": "Interrupt Processing",
    "interrupt_preemption": "Interrupt Preemption Processing",
    "message": "Message Processing",
    "synchronization": "Synchronization Processing",
    "memory_allocation": "RTOS memory allocation",
}


def parse_args():
    parser = argparse.ArgumentParser(
        allow_abbrev=False, formatter_class=argparse.RawTextHelpFormatter, description=__doc__
    )
    parser.add_argument(
        'files',
        metavar='TWISTER_JSON_FILE',
        nargs='+',
        help='twister.json report file to update with additional attributes.',
    )
    args = parser.parse_args()
    return args


def preprocess(json_report, fname):
    global has_errors
    process_cnt = 0

    if 'testsuites' not in json_report:
        print(f"ERROR: not a twister.json report: {fname}")
        has_errors += 1
        return None

    if not json_report['testsuites']:
        print(f"WARNING: no testsuites in {fname}")
        return None

    # 1-st pass to extract all baseline measurements from the report:
    # it must be only one per platform.
    basic_metrics = {}
    for ts in json_report['testsuites']:
        if (
            ts['name'].endswith(threadx_benchmark_name_prefix + ".basic")
            and ts['status'] == 'passed'
        ):
            metric_of = (ts['platform'], ts['toolchain'])
            if metric_of in basic_metrics:
                print(f"ERROR: {fname} has duplicated basic metrics for {metric_of}")
                has_errors += 1
                return None
            all_baselines = []
            for rec in ts['recording']:
                all_baselines += rec['total_time_period']
            basic_metrics[metric_of] = statistics.median(map(int, all_baselines))

    # 2-nd pass: add benchmark name and normalized metrics including the baseline itself
    for ts in json_report['testsuites']:
        idx = ts['name'].find(threadx_benchmark_name_prefix)
        if idx < 0:
            continue
        suff = ts['name'][idx + len(threadx_benchmark_name_prefix) + 1 :]
        if suff not in threadx_benchmark_name:
            continue
        metric_of = (ts['platform'], ts['toolchain'])
        if metric_of not in basic_metrics:
            print(f"DEBUG: {fname} skip {metric_of} {ts['name']} has no baseline metrics")
            continue
        bl_median = basic_metrics[metric_of]
        for rec in ts['recording']:
            rec['benchmark_name'] = threadx_benchmark_name[suff]
            rec['total_time_period_median'] = statistics.median(map(int, rec['total_time_period']))
            rec['baseline_median'] = bl_median
            rec['total_time_period_normalized'] = [
                int(x) / bl_median for x in rec['total_time_period']
            ]
            process_cnt += 1
            print(f"INFO: {fname} update {metric_of} {ts['name']}")

    print(f"INFO: {process_cnt} suites updated in {fname}")
    return json_report


def main():
    global has_errors

    args = parse_args()

    for fname in args.files:
        if not os.path.exists(fname):
            print(f"ERROR: not found {fname}")
            has_errors += 1
        else:
            with open(fname) as in_file:
                try:
                    print(f"INFO: process {fname}")
                    report = json.load(in_file)
                    in_file.close()
                    report = preprocess(report, fname)
                    if report:
                        with open(fname, "w") as out_file:
                            json.dump(report, out_file, indent=4, separators=(',', ':'))
                            print(f"INFO: processed {fname}")
                    else:
                        print(f"INFO: skipped {fname}")
                except json.JSONDecodeError as json_error:
                    print(f"ERROR: in {fname} JSON: {json_error}")
                    has_errors += 1
    return has_errors


if __name__ == '__main__':
    sys.exit(main())
