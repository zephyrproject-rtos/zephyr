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

    # Visualize the results as an SVG bar chart (optionally baseline vs
    # current when --baseline is also given).
    samples/net/zperf/scripts/zperf_regression.py --base-dir .. \\
        --twister-json ../build/zperf_cur/twister.json \\
        --baseline baseline.json --plot throughput.svg
"""

import argparse
import json
import math
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
    print(f"{'metric':<20}{'baseline':>12}{'current':>12}{'change':>10}  status")
    for metric in sorted(set(baseline) | set(current)):
        base = baseline.get(metric)
        cur = current.get(metric)
        if base is None or cur is None:
            print(f"{metric:<20}{str(base):>12}{str(cur):>12}{'':>10}  MISSING")
            ok = False
            continue

        change_pct = ((cur - base) / base * 100.0) if base else 0.0
        min_allowed = base * (1.0 - tolerance_pct / 100.0)
        status = "OK" if cur >= min_allowed else "REGRESSION"
        if status != "OK":
            ok = False
        print(f"{metric:<20}{base:>12.3f}{cur:>12.3f}{change_pct:>+9.1f}%  {status}")

    return ok


def _nice_ceil(value: float) -> float:
    """Round a positive value up to a 1/2/2.5/5/10 * 10^n axis maximum."""
    if value <= 0:
        return 1.0
    exp = math.floor(math.log10(value))
    base = 10.0**exp
    for mult in (1.0, 2.0, 2.5, 5.0, 10.0):
        if value <= mult * base:
            return mult * base
    return 10.0 * base


def write_plot(
    path: str, current: dict[str, float], baseline: dict[str, float] | None = None
) -> None:
    """Render a bar chart of the metrics as a self-contained SVG file.

    Without a baseline a single bar per metric is drawn; with a baseline the
    baseline and current values are drawn as grouped bars per metric.
    """
    metrics = sorted(set(current) | (set(baseline) if baseline else set()))

    margin_left, margin_right, margin_top, margin_bottom = 70, 30, 60, 110
    group_w = 70
    plot_w = group_w * max(len(metrics), 1)
    height = 460
    plot_h = height - margin_top - margin_bottom
    width = margin_left + plot_w + margin_right
    plot_bottom = margin_top + plot_h

    values = [current.get(m, 0.0) for m in metrics]
    if baseline:
        values += [baseline.get(m, 0.0) for m in metrics]
    axis_max = _nice_ceil(max(values) if values else 1.0)

    def y_of(val: float) -> float:
        return plot_bottom - (val / axis_max) * plot_h

    color_cur = "#1f77b4"
    color_base = "#999999"
    svg: list[str] = []
    svg.append(
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" '
        f'height="{height}" font-family="sans-serif" font-size="12">'
    )
    svg.append(f'<rect width="{width}" height="{height}" fill="white"/>')
    svg.append(
        f'<text x="{width / 2:.1f}" y="24" text-anchor="middle" '
        f'font-size="16" font-weight="bold">'
        f'zperf loopback throughput (Mbps)</text>'
    )

    ticks = 5
    for i in range(ticks + 1):
        val = axis_max * i / ticks
        y = y_of(val)
        svg.append(
            f'<line x1="{margin_left}" y1="{y:.1f}" '
            f'x2="{margin_left + plot_w}" y2="{y:.1f}" stroke="#e0e0e0"/>'
        )
        svg.append(
            f'<text x="{margin_left - 8}" y="{y + 4:.1f}" '
            f'text-anchor="end" fill="#555">{val:.1f}</text>'
        )

    svg.append(
        f'<line x1="{margin_left}" y1="{margin_top}" x2="{margin_left}" '
        f'y2="{plot_bottom}" stroke="#333"/>'
    )
    svg.append(
        f'<line x1="{margin_left}" y1="{plot_bottom}" '
        f'x2="{margin_left + plot_w}" y2="{plot_bottom}" stroke="#333"/>'
    )

    def bar(x: float, val: float, bar_w: float, color: str) -> None:
        y = y_of(val)
        svg.append(
            f'<rect x="{x:.1f}" y="{y:.1f}" width="{bar_w:.1f}" '
            f'height="{plot_bottom - y:.1f}" fill="{color}"/>'
        )
        svg.append(
            f'<text x="{x + bar_w / 2:.1f}" y="{y - 4:.1f}" '
            f'text-anchor="middle" font-size="9" fill="{color}">'
            f'{val:.1f}</text>'
        )

    for idx, metric in enumerate(metrics):
        center = margin_left + idx * group_w + group_w / 2
        if baseline:
            bar_w = group_w * 0.30
            if metric in baseline:
                bar(center - bar_w - 2, baseline[metric], bar_w, color_base)
            if metric in current:
                bar(center + 2, current[metric], bar_w, color_cur)
        else:
            bar_w = group_w * 0.5
            bar(center - bar_w / 2, current.get(metric, 0.0), bar_w, color_cur)

        label_y = plot_bottom + 14
        svg.append(
            f'<text x="{center:.1f}" y="{label_y:.1f}" '
            f'transform="rotate(-45 {center:.1f} {label_y:.1f})" '
            f'text-anchor="end" fill="#333">{metric}</text>'
        )

    if baseline:
        lx, ly = margin_left + 10, margin_top + 6
        svg.append(f'<rect x="{lx}" y="{ly}" width="12" height="12" fill="{color_base}"/>')
        svg.append(f'<text x="{lx + 18}" y="{ly + 11}">baseline</text>')
        svg.append(f'<rect x="{lx + 90}" y="{ly}" width="12" height="12" fill="{color_cur}"/>')
        svg.append(f'<text x="{lx + 108}" y="{ly + 11}">current</text>')

    svg.append("</svg>")
    with open(path, "w") as fp:
        fp.write("\n".join(svg) + "\n")
    print(f"Wrote plot to {path}")


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
        "--plot",
        metavar="PATH",
        help="Write an SVG bar chart of the metrics to PATH "
        "(grouped baseline vs current when --baseline is "
        "given)",
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

    baseline = None
    if args.baseline:
        baseline_path = validate_path(args.baseline, args.base_dir, for_write=False)
        with open(baseline_path) as fp:
            baseline = {k: float(v) for k, v in json.load(fp).items()}

    if args.plot:
        plot_path = validate_path(args.plot, args.base_dir, for_write=True)
        write_plot(plot_path, current, baseline)

    if baseline is not None:
        print()
        if not compare(baseline, current, args.tolerance):
            print("\nThroughput regression detected.", file=sys.stderr)
            return 1
        print("\nNo throughput regression.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
