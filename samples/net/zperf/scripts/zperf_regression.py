#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors

"""Capture and compare zperf loopback throughput from a twister run.

The ``sample.net.zperf.loopback_icount`` scenario runs a deterministic,
host-speed-independent throughput test under QEMU icount mode and records the
UDP and TCP throughput (in Mbps) into the twister JSON report.

By default all files read or written must live under the current directory;
pass --base-dir to widen that (the examples below read the twister report from
the sibling ../build tree and write into the repo, so they use --base-dir ..).

Typical usage::

    # Capture a baseline on the reference commit.
    ./scripts/twister -p qemu_x86 -s sample.net.zperf.loopback_icount \\
        --outdir ../build/zperf_base
    samples/net/zperf/scripts/zperf_regression.py --base-dir .. \\
        --twister-json ../build/zperf_base/twister.json \\
        --save baseline.json

    # On a later commit, re-run and gate on a maximum allowed drop.
    ./scripts/twister -p qemu_x86 -s sample.net.zperf.loopback_icount \\
        --outdir ../build/zperf_cur
    samples/net/zperf/scripts/zperf_regression.py --base-dir .. \\
        --twister-json ../build/zperf_cur/twister.json \\
        --baseline baseline.json --tolerance 5
"""

import argparse
import json
import os
import sys

DEFAULT_SUITE = "sample.net.zperf.loopback_icount"


def validate_path(path: str, base_dir: str, *, for_write: bool) -> str:
    """Resolve *path* and ensure it stays inside *base_dir*.

    All files this script reads or writes are taken from CLI arguments. To
    avoid path-traversal or absolute-path escapes when the script is driven
    with untrusted or faulty arguments, every such path is resolved (following
    symlinks and ``..``) and rejected unless it lives under the resolved base
    directory. The validated absolute path is returned for use with ``open()``.
    """
    if not path or "\x00" in path:
        sys.exit(f"Invalid path: {path!r}")

    base_real = os.path.realpath(base_dir)
    if not os.path.isdir(base_real):
        sys.exit(f"Base directory '{base_dir}' does not exist.")

    # Resolve the path as the user means it (relative paths against the current
    # directory, absolute paths as-is, following symlinks and ".."), then
    # require the result to stay under the resolved base directory.
    target_real = os.path.realpath(path)

    if os.path.commonpath([base_real, target_real]) != base_real:
        sys.exit(
            f"Refusing to access '{path}': resolved path '{target_real}' is "
            f"outside the permitted base directory '{base_real}'. Use "
            f"--base-dir to widen the allowed location."
        )

    if for_write:
        parent = os.path.dirname(target_real)
        if not os.path.isdir(parent):
            sys.exit(f"Cannot write '{path}': '{parent}' is not a directory.")
        if os.path.isdir(target_real):
            sys.exit(f"Cannot write '{path}': it is a directory.")

    return target_real


def extract_metrics(twister_json: str, suite_name: str) -> dict[str, float]:
    """Return a {metric: value} mapping from a twister.json recording."""
    with open(twister_json) as fp:
        data = json.load(fp)

    metrics: dict[str, float] = {}
    for suite in data.get("testsuites", []):
        if suite.get("name") != suite_name:
            continue
        for record in suite.get("recording") or []:
            for key, value in record.items():
                try:
                    metrics[key] = float(value)
                except (TypeError, ValueError):
                    continue

    if not metrics:
        sys.exit(
            f"No throughput recordings for suite '{suite_name}' in "
            f"{twister_json}. Did the run pass?"
        )

    return metrics


def compare(baseline: dict[str, float], current: dict[str, float], tolerance_pct: float) -> bool:
    """Print a comparison table and return True if no metric regressed."""
    ok = True
    print(f"{'metric':<12}{'baseline':>12}{'current':>12}{'change':>10}  status")
    for metric in sorted(set(baseline) | set(current)):
        base = baseline.get(metric)
        cur = current.get(metric)
        if base is None or cur is None:
            print(f"{metric:<12}{str(base):>12}{str(cur):>12}{'':>10}  MISSING")
            ok = False
            continue

        change_pct = ((cur - base) / base * 100.0) if base else 0.0
        min_allowed = base * (1.0 - tolerance_pct / 100.0)
        status = "OK" if cur >= min_allowed else "REGRESSION"
        if status != "OK":
            ok = False
        print(f"{metric:<12}{base:>12.3f}{cur:>12.3f}{change_pct:>+9.1f}%  {status}")

    return ok


def main() -> int:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )
    parser.add_argument(
        "--twister-json", required=True, help="Path to the twister.json of the current run"
    )
    parser.add_argument(
        "--suite", default=DEFAULT_SUITE, help="Test suite name to read recordings from"
    )
    parser.add_argument(
        "--save", metavar="PATH", help="Write the extracted metrics as a baseline file"
    )
    parser.add_argument(
        "--baseline", metavar="PATH", help="Compare against a previously saved baseline file"
    )
    parser.add_argument(
        "--tolerance",
        type=float,
        default=5.0,
        help="Allowed throughput drop in percent (default: 5.0)",
    )
    parser.add_argument(
        "--base-dir",
        metavar="DIR",
        default=".",
        help="Restrict all files read or written to this "
        "directory tree (default: current directory). "
        "Paths resolving outside it are rejected.",
    )
    args = parser.parse_args()

    twister_json = validate_path(args.twister_json, args.base_dir, for_write=False)
    current = extract_metrics(twister_json, args.suite)

    for metric, value in sorted(current.items()):
        print(f"{metric} = {value:.3f} Mbps")

    if args.save:
        save_path = validate_path(args.save, args.base_dir, for_write=True)
        with open(save_path, "w") as fp:
            json.dump(current, fp, indent=4, sort_keys=True)
            fp.write("\n")
        print(f"Saved baseline to {save_path}")

    if args.baseline:
        baseline_path = validate_path(args.baseline, args.base_dir, for_write=False)
        with open(baseline_path) as fp:
            baseline = {k: float(v) for k, v in json.load(fp).items()}
        print()
        if not compare(baseline, current, args.tolerance):
            print("\nThroughput regression detected.", file=sys.stderr)
            return 1
        print("\nNo throughput regression.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
