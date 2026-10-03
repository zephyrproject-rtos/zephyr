#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors

"""Capture and compare zperf loopback throughput from a twister run.

The ``sample.net.zperf.loopback_icount`` scenario runs a deterministic,
host-speed-independent throughput test under QEMU icount mode and records the
UDP and TCP throughput (in Mbps) into the twister JSON report.

Metrics are keyed by twister platform, so a single run covering several
platforms (for example ``-p qemu_x86 -p qemu_x86_64``) is reported per
platform rather than merged: the 32-bit and 64-bit numbers are not comparable
with each other.

By default all files read or written must live under the current directory;
pass --base-dir to widen that (the examples below read the twister report from
the sibling ../build tree and write into the repo, so they use --base-dir ..).

Typical usage::

    # Capture a baseline on the reference commit.
    ./scripts/twister -T samples/net/zperf -s sample.net.zperf.loopback_icount \\
        --outdir ../build/zperf_base
    samples/net/zperf/scripts/zperf_regression.py --base-dir .. \\
        --twister-json ../build/zperf_base/twister.json \\
        --save baseline.json

    # On a later commit, re-run and gate on a maximum allowed drop.
    ./scripts/twister -T samples/net/zperf -s sample.net.zperf.loopback_icount \\
        --outdir ../build/zperf_cur
    samples/net/zperf/scripts/zperf_regression.py --base-dir .. \\
        --twister-json ../build/zperf_cur/twister.json \\
        --baseline baseline.json --tolerance 5

    # Compare two twister runs directly, and report the difference without
    # ever failing (what continuous integration does).
    samples/net/zperf/scripts/zperf_regression.py --base-dir .. \\
        --twister-json ../build/zperf_cur/twister.json \\
        --baseline-twister-json ../build/zperf_base/twister.json \\
        --markdown report.md --annotate --exit-zero

    # Visualize the results as an SVG bar chart (optionally baseline vs
    # current when a baseline is also given).
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

# Metrics that moved by less than this are reported as unchanged. The icount
# measurement repeats exactly for a given binary, but a baseline and a modified
# tree are two different binaries: adding or removing code shifts the layout and
# nudges the number without changing the work actually done. See the "Noise
# floor" discussion in README-loopback-icount.rst.
DEFAULT_THRESHOLD_PCT = 1.0

# Placeholder platform name used for baseline files written before the metrics
# were keyed by platform.
UNKNOWN_PLATFORM = "unknown"


class MetricsError(Exception):
    """No usable throughput recordings could be read."""


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


def platform_matches(platform: str, wanted: str) -> bool:
    """Match a twister platform name against a user-supplied selector.

    Twister reports a platform as ``board/soc`` (``qemu_x86/atom``), so accept
    either the full name or just the board part.
    """
    return wanted in (platform, platform.split("/", 1)[0])


def extract_metrics(
    twister_json: str, suite_name: str, platform: str | None = None
) -> dict[str, dict[str, float]]:
    """Return a {platform: {metric: value}} mapping from a twister.json recording."""
    try:
        with open(twister_json) as fp:
            data = json.load(fp)
    except OSError as err:
        raise MetricsError(f"Cannot read {twister_json}: {err}") from err
    except json.JSONDecodeError as err:
        raise MetricsError(f"{twister_json} is not valid JSON: {err}") from err

    metrics: dict[str, dict[str, float]] = {}
    for suite in data.get("testsuites", []):
        if suite.get("name") != suite_name:
            continue

        suite_platform = suite.get("platform") or UNKNOWN_PLATFORM
        if platform is not None and not platform_matches(suite_platform, platform):
            continue

        for record in suite.get("recording") or []:
            for key, value in record.items():
                try:
                    metrics.setdefault(suite_platform, {})[key] = float(value)
                except (TypeError, ValueError):
                    continue

    if not metrics:
        selected = f" on platform '{platform}'" if platform else ""
        raise MetricsError(
            f"No throughput recordings for suite '{suite_name}'{selected} in "
            f"{twister_json}. Did the run pass?"
        )

    return metrics


def load_baseline(path: str) -> dict[str, dict[str, float]]:
    """Read a saved baseline file, accepting the flat pre-platform format too."""
    with open(path) as fp:
        raw = json.load(fp)

    if not isinstance(raw, dict):
        raise MetricsError(f"{path} does not contain a baseline object.")

    # A baseline saved before the metrics were keyed by platform is a flat
    # {metric: value} mapping. Treat the whole file as one unnamed platform so
    # existing baseline files keep working.
    if raw and all(not isinstance(value, dict) for value in raw.values()):
        return {UNKNOWN_PLATFORM: {k: float(v) for k, v in raw.items()}}

    return {
        platform: {k: float(v) for k, v in values.items()}
        for platform, values in raw.items()
        if isinstance(values, dict)
    }


def pair_platforms(
    baseline: dict[str, dict[str, float]], current: dict[str, dict[str, float]]
) -> list[tuple[str, dict[str, float], dict[str, float]]]:
    """Line the two runs' platforms up for comparison.

    A baseline in the old flat format carries no platform name, so pair it with
    the current run's single platform when there is exactly one.
    """
    if list(baseline) == [UNKNOWN_PLATFORM] and len(current) == 1:
        platform, values = next(iter(current.items()))
        return [(platform, baseline[UNKNOWN_PLATFORM], values)]

    return [
        (platform, baseline.get(platform, {}), current.get(platform, {}))
        for platform in sorted(set(baseline) | set(current))
    ]


def compare_platform(
    baseline: dict[str, float],
    current: dict[str, float],
    tolerance_pct: float,
    threshold_pct: float,
) -> list[dict]:
    """Return one row per metric describing how it moved."""
    rows = []
    for metric in sorted(set(baseline) | set(current)):
        base = baseline.get(metric)
        cur = current.get(metric)

        if base is None or cur is None:
            rows.append({"metric": metric, "base": base, "current": cur, "status": "MISSING"})
            continue

        change_pct = ((cur - base) / base * 100.0) if base else 0.0
        min_allowed = base * (1.0 - tolerance_pct / 100.0)

        if abs(change_pct) < threshold_pct:
            verdict = "unchanged"
        elif change_pct > 0:
            verdict = "improved"
        else:
            verdict = "slower"

        rows.append(
            {
                "metric": metric,
                "base": base,
                "current": cur,
                "change": change_pct,
                "verdict": verdict,
                "status": "OK" if cur >= min_allowed else "REGRESSION",
            }
        )

    return rows


def rows_ok(rows: list[dict]) -> bool:
    """True when no metric is missing and none dropped past the tolerance."""
    return all(row["status"] == "OK" for row in rows)


def print_comparison(results: list[tuple[str, list[dict]]]) -> None:
    """Print a comparison table per platform."""
    for platform, rows in results:
        print(f"\nPlatform: {platform}")
        print(f"{'metric':<20}{'baseline':>12}{'current':>12}{'change':>10}  status")
        for row in rows:
            if row["status"] == "MISSING":
                base = "-" if row["base"] is None else f"{row['base']:.3f}"
                cur = "-" if row["current"] is None else f"{row['current']:.3f}"
                print(f"{row['metric']:<20}{base:>12}{cur:>12}{'':>10}  MISSING")
                continue
            print(
                f"{row['metric']:<20}{row['base']:>12.3f}{row['current']:>12.3f}"
                f"{row['change']:>+9.1f}%  {row['status']}"
            )


def _verdict_icon(verdict: str) -> str:
    return {"improved": "🟢", "slower": "🔴", "unchanged": "▫️"}.get(verdict, "❔")


def _platform_headline(rows: list[dict], threshold_pct: float) -> str:
    """One line saying what moved on a platform, or that nothing did."""
    moved = [r for r in rows if r["status"] != "MISSING" and r["verdict"] != "unchanged"]
    missing = [r for r in rows if r["status"] == "MISSING"]

    if moved:
        headline = ", ".join(f"{r['metric']} {r['change']:+.1f}%" for r in moved)
    else:
        headline = f"no metric moved by more than {threshold_pct:g}%"

    if missing:
        headline += f" ({len(missing)} not measured)"

    return headline


def write_markdown(
    path: str,
    results: list[tuple[str, list[dict]]],
    threshold_pct: float,
    baseline_label: str,
    current_label: str,
) -> None:
    """Write the comparison as markdown, for a GitHub Actions job summary."""
    out: list[str] = ["### Networking throughput", ""]

    for platform, rows in results:
        # Collapse a platform whose metrics all held still. The report is also
        # posted as a pull request comment, where sixteen unchanged rows on
        # every networking pull request would be noise; the headline says what
        # happened and the table is one click away.
        headline = _platform_headline(rows, threshold_pct)
        moved = any(r["status"] != "MISSING" and r["verdict"] != "unchanged" for r in rows)
        out.append(
            f"<details{' open' if moved else ''}><summary><b>{platform}</b> — {headline}</summary>"
        )
        out.append("")
        out.append(f"| metric | {baseline_label} | {current_label} | change | |")
        out.append("| --- | ---: | ---: | ---: | :-: |")
        for row in rows:
            if row["status"] == "MISSING":
                base = "-" if row["base"] is None else f"{row['base']:.3f}"
                cur = "-" if row["current"] is None else f"{row['current']:.3f}"
                out.append(f"| `{row['metric']}` | {base} | {cur} | not measured | ❔ |")
                continue
            out.append(
                f"| `{row['metric']}` | {row['base']:.3f} | {row['current']:.3f} "
                f"| {row['change']:+.1f}% | {_verdict_icon(row['verdict'])} |"
            )
        out.append("")
        out.append("</details>")
        out.append("")

    out.append(
        f"Throughput is measured in Mbps of virtual time under QEMU icount, which makes "
        f"it a stand-in for instructions per payload byte rather than a line rate. "
        f"Changes below {threshold_pct:g}% are reported as unchanged: the two trees are "
        f"different binaries, so code layout alone moves the number a little."
    )
    out.append("")
    out.append("This check is informational and never fails a pull request.")
    out.append("")

    with open(path, "w") as fp:
        fp.write("\n".join(out))
    print(f"Wrote markdown report to {path}")


def write_unavailable_markdown(path: str, reason: str) -> None:
    """Write a markdown note explaining why no comparison could be made."""
    with open(path, "w") as fp:
        fp.write(
            "### Networking throughput\n\n"
            f"No comparison available: {reason}\n\n"
            "This check is informational and never fails a pull request.\n"
        )
    print(f"Wrote markdown report to {path}")


def emit_annotations(results: list[tuple[str, list[dict]]], threshold_pct: float) -> None:
    """Emit one ``::notice::`` workflow command per platform for the Actions log.

    The platform is named in the message as well as in the title, because the
    Actions log renders only the message: without it, two platforms that both
    came back unchanged produce two identical lines.
    """
    for platform, rows in results:
        detail = _platform_headline(rows, threshold_pct)
        print(f"::notice title=Networking throughput ({platform})::{platform}: {detail}")


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
    path: str,
    current: dict[str, float],
    baseline: dict[str, float] | None = None,
    title: str = "zperf loopback throughput (Mbps)",
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
        f'font-size="16" font-weight="bold">{title}</text>'
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


def plot_path_for(path: str, platform: str, single: bool) -> str:
    """Derive a per-platform plot file name when more than one platform ran."""
    if single:
        return path

    stem, ext = os.path.splitext(path)
    suffix = platform.replace("/", "_")
    return f"{stem}-{suffix}{ext or '.svg'}"


def parse_args() -> argparse.Namespace:
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
        "--platform",
        metavar="NAME",
        help="Only consider this twister platform, given either as the full "
        "'board/soc' name or as just the board (default: all platforms in "
        "the report, reported separately)",
    )
    parser.add_argument(
        "--save", metavar="PATH", help="Write the extracted metrics as a baseline file"
    )
    parser.add_argument(
        "--baseline", metavar="PATH", help="Compare against a previously saved baseline file"
    )
    parser.add_argument(
        "--baseline-twister-json",
        metavar="PATH",
        help="Compare against the twister.json of another run, without going "
        "through a saved baseline file",
    )
    parser.add_argument(
        "--tolerance",
        type=float,
        default=5.0,
        help="Allowed throughput drop in percent before the exit status "
        "reports a regression (default: 5.0)",
    )
    parser.add_argument(
        "--threshold",
        type=float,
        default=DEFAULT_THRESHOLD_PCT,
        help="Reporting noise floor in percent: a metric that moved by less "
        f"than this is described as unchanged (default: {DEFAULT_THRESHOLD_PCT}). "
        "The baseline and the current tree are different binaries, so code "
        "layout alone moves the number slightly without any change in the "
        "work performed",
    )
    parser.add_argument(
        "--markdown",
        metavar="PATH",
        help="Write the comparison as a markdown report, for example to append "
        "to a GitHub Actions job summary",
    )
    parser.add_argument(
        "--annotate",
        action="store_true",
        help="Emit ::notice:: workflow command annotations summarising each platform",
    )
    parser.add_argument(
        "--exit-zero",
        action="store_true",
        help="Always exit successfully, even when a metric regressed. Use when "
        "the comparison is informational and must not fail a build",
    )
    parser.add_argument(
        "--plot",
        metavar="PATH",
        help="Write an SVG bar chart of the metrics to PATH "
        "(grouped baseline vs current when a baseline is "
        "given). With several platforms the platform name is "
        "appended to each file name",
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

    if args.baseline and args.baseline_twister_json:
        parser.error("--baseline and --baseline-twister-json are mutually exclusive")

    return args


def main() -> int:
    args = parse_args()

    markdown_path = None
    if args.markdown:
        markdown_path = validate_path(args.markdown, args.base_dir, for_write=True)

    twister_json = validate_path(args.twister_json, args.base_dir, for_write=False)
    try:
        current = extract_metrics(twister_json, args.suite, args.platform)
    except MetricsError as err:
        # Without the current run there is nothing to report at all. Still
        # leave a readable note behind when a report was asked for.
        if markdown_path:
            write_unavailable_markdown(markdown_path, str(err))
        print(err, file=sys.stderr)
        return 0 if args.exit_zero else 1

    for platform, values in sorted(current.items()):
        print(f"Platform: {platform}")
        for metric, value in sorted(values.items()):
            print(f"  {metric} = {value:.3f} Mbps")

    if args.save:
        save_path = validate_path(args.save, args.base_dir, for_write=True)
        with open(save_path, "w") as fp:
            json.dump(current, fp, indent=4, sort_keys=True)
            fp.write("\n")
        print(f"Saved baseline to {save_path}")

    baseline = None
    baseline_error = None
    if args.baseline or args.baseline_twister_json:
        try:
            if args.baseline:
                path = validate_path(args.baseline, args.base_dir, for_write=False)
                baseline = load_baseline(path)
            else:
                path = validate_path(args.baseline_twister_json, args.base_dir, for_write=False)
                baseline = extract_metrics(path, args.suite, args.platform)
        except (MetricsError, OSError, json.JSONDecodeError) as err:
            baseline_error = str(err)

    if baseline_error is not None:
        # A missing or unusable baseline is expected in continuous integration
        # when the reference tree failed to build. Say so and carry on.
        print(f"No baseline to compare against: {baseline_error}", file=sys.stderr)
        if markdown_path:
            write_unavailable_markdown(markdown_path, baseline_error)
        if args.annotate:
            print(f"::notice title=Networking throughput::{baseline_error}")
        return 0 if args.exit_zero else 1

    if baseline is None:
        pairs = [(platform, {}, values) for platform, values in sorted(current.items())]
    else:
        pairs = pair_platforms(baseline, current)

    if args.plot:
        single = len(pairs) == 1
        for platform, base_values, values in pairs:
            plot_path = validate_path(
                plot_path_for(args.plot, platform, single), args.base_dir, for_write=True
            )
            write_plot(
                plot_path,
                values,
                base_values or None,
                title=f"zperf loopback throughput, {platform} (Mbps)",
            )

    if baseline is None:
        return 0

    results = [
        (platform, compare_platform(base, cur, args.tolerance, args.threshold))
        for platform, base, cur in pairs
    ]

    print_comparison(results)

    if markdown_path:
        write_markdown(markdown_path, results, args.threshold, "baseline", "current")

    if args.annotate:
        emit_annotations(results, args.threshold)

    ok = all(rows_ok(rows) for _, rows in results)
    if not ok:
        print("\nThroughput regression detected.", file=sys.stderr)
        return 0 if args.exit_zero else 1

    print("\nNo throughput regression.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
