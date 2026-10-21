#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Compare two runs of the kernel latency benchmark.

The ``benchmark.kernel.latency.*`` scenarios record one percentile row per
benchmark into the twister JSON report. This script reads two such reports,
lines the benchmarks up by platform, scenario and name, and reports which of
them moved.

Under QEMU icount the cycle counter advances with executed instructions, so a
given binary reports the same numbers on every run and on every host. Two
different trees are two different binaries, and a benchmark that moved is a
benchmark whose code path executes a different number of instructions.

Typical usage::

    # Measure the reference tree and the tree under test.
    ./scripts/twister -T tests/benchmarks/kernel_latency -p qemu_x86 \\
        --outdir ../build/latency_base
    ./scripts/twister -T tests/benchmarks/kernel_latency -p qemu_x86 \\
        --outdir ../build/latency_cur

    # Compare the two, and write a markdown report.
    tests/benchmarks/kernel_latency/scripts/kernel_latency_regression.py \\
        --base-dir .. --twister-json ../build/latency_cur/twister.json \\
        --baseline-twister-json ../build/latency_base/twister.json \\
        --markdown report.md

    # Keep a baseline file around instead of a whole twister report.
    tests/benchmarks/kernel_latency/scripts/kernel_latency_regression.py \\
        --base-dir .. --twister-json ../build/latency_base/twister.json \\
        --save baseline.json
    tests/benchmarks/kernel_latency/scripts/kernel_latency_regression.py \\
        --base-dir .. --twister-json ../build/latency_cur/twister.json \\
        --baseline baseline.json --tolerance 5
"""

import argparse
import json
import os
import sys

DEFAULT_SCENARIO_PREFIX = "benchmark.kernel.latency."

# The column of the percentile row to compare. The median is the steady
# state cost of a short, uncontended kernel path; the tail columns show
# what an occasional interrupt costs and are not what a code change moves.
DEFAULT_STATISTIC = "p50"

STATISTICS = ("min", "p50", "p90", "p99", "p999", "p9999", "max")

# A benchmark that moved by less than this is reported as unchanged. The
# numbers are deterministic for a given binary, but the two trees compared
# are different binaries, and a few cycles of difference on a short path is
# the alignment of the code around it rather than the path itself.
DEFAULT_THRESHOLD_PCT = 2.0

# How many moved benchmarks a platform headline names before it says "and N
# more". The full table is right below it.
HEADLINE_LIMIT = 6

# The scenarios are the ready queue algorithms, and the README discusses the
# other two against SCHED_SIMPLE, so put it first. Anything else follows in
# name order.
SCENARIO_ORDER = ("simple", "scalable", "multiq")

# {platform: {scenario: {benchmark: value}}}
Metrics = dict[str, dict[str, dict[str, float]]]


class MetricsError(Exception):
    """No usable benchmark recordings could be read."""


def validate_path(path: str, base_dir: str, *, for_write: bool) -> str:
    """Resolve *path* and require it to stay inside *base_dir*.

    Every file this script reads or writes is named on the command line, so
    each is resolved and rejected unless it lives under the base directory.
    """
    if not path or "\x00" in path:
        sys.exit(f"Invalid path: {path!r}")

    base_real = os.path.realpath(base_dir)
    if not os.path.isdir(base_real):
        sys.exit(f"Base directory '{base_dir}' does not exist.")

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
    """Match a twister platform (``qemu_x86/atom``) against a board or full name."""
    return wanted in (platform, platform.split("/", 1)[0])


def extract_metrics(
    twister_json: str, prefix: str, statistic: str, platform: str | None = None
) -> Metrics:
    """Read the chosen statistic of every benchmark out of a twister report."""
    try:
        with open(twister_json) as fp:
            data = json.load(fp)
    except OSError as err:
        raise MetricsError(f"Cannot read {twister_json}: {err}") from err
    except json.JSONDecodeError as err:
        raise MetricsError(f"{twister_json} is not valid JSON: {err}") from err

    metrics: Metrics = {}
    for suite in data.get("testsuites", []):
        name = suite.get("name") or ""
        if not name.startswith(prefix):
            continue

        suite_platform = suite.get("platform") or "unknown"
        if platform is not None and not platform_matches(suite_platform, platform):
            continue

        # The scenario is what distinguishes the builds of one benchmark
        # from each other: here, the ready queue algorithm.
        scenario = name[len(prefix) :].rsplit(".", 1)[-1]

        for record in suite.get("recording") or []:
            try:
                benchmark = f"{record['suite']}.{record['metric']}"
                value = float(record[statistic])
            except (KeyError, TypeError, ValueError):
                continue
            # A target whose cycle counter the emulator does not implement
            # reports every benchmark as zero. That is no measurement.
            if value <= 0.0:
                continue
            metrics.setdefault(suite_platform, {}).setdefault(scenario, {})[benchmark] = value

    if not metrics:
        selected = f" on platform '{platform}'" if platform else ""
        raise MetricsError(
            f"No benchmark recordings for '{prefix}*'{selected} in {twister_json}. "
            "Did the run pass?"
        )

    return metrics


def load_baseline(path: str) -> Metrics:
    """Read a baseline written by --save."""
    with open(path) as fp:
        raw = json.load(fp)

    if not isinstance(raw, dict):
        raise MetricsError(f"{path} does not contain a baseline object.")

    return {
        platform: {
            scenario: {name: float(value) for name, value in benchmarks.items()}
            for scenario, benchmarks in scenarios.items()
            if isinstance(benchmarks, dict)
        }
        for platform, scenarios in raw.items()
        if isinstance(scenarios, dict)
    }


def compare(
    baseline: dict[str, float],
    current: dict[str, float],
    tolerance_pct: float,
    threshold_pct: float,
) -> list[dict]:
    """Return one row per benchmark describing how it moved."""
    rows = []
    for name in sorted(set(baseline) | set(current)):
        base = baseline.get(name)
        cur = current.get(name)

        if base is None or cur is None:
            rows.append({"name": name, "base": base, "current": cur, "status": "MISSING"})
            continue

        change_pct = ((cur - base) / base * 100.0) if base else 0.0
        max_allowed = base * (1.0 + tolerance_pct / 100.0)

        # Latency: lower is better.
        if abs(change_pct) < threshold_pct:
            verdict = "unchanged"
        elif change_pct < 0:
            verdict = "faster"
        else:
            verdict = "slower"

        rows.append(
            {
                "name": name,
                "base": base,
                "current": cur,
                "change": change_pct,
                "verdict": verdict,
                "status": "OK" if cur <= max_allowed else "REGRESSION",
            }
        )

    return rows


def scenario_sort_key(scenario: str) -> tuple[int, str]:
    if scenario in SCENARIO_ORDER:
        return (SCENARIO_ORDER.index(scenario), scenario)
    return (len(SCENARIO_ORDER), scenario)


def compare_all(
    baseline: Metrics, current: Metrics, tolerance_pct: float, threshold_pct: float
) -> dict[str, dict[str, list[dict]]]:
    """Compare every platform and scenario present in either run.

    A platform that one of the runs did not measure at all, because it failed
    to build or to boot there, is kept with no scenarios: the report says so
    in one line rather than listing every benchmark as missing.
    """
    results: dict[str, dict[str, list[dict]]] = {}
    for platform in sorted(set(baseline) | set(current)):
        base_scenarios = baseline.get(platform, {})
        cur_scenarios = current.get(platform, {})
        results[platform] = {}
        if not base_scenarios or not cur_scenarios:
            continue
        for scenario in sorted(set(base_scenarios) | set(cur_scenarios), key=scenario_sort_key):
            results.setdefault(platform, {})[scenario] = compare(
                base_scenarios.get(scenario, {}),
                cur_scenarios.get(scenario, {}),
                tolerance_pct,
                threshold_pct,
            )
    return results


def moved_rows(scenarios: dict[str, list[dict]]) -> list[tuple[str, dict]]:
    """The (scenario, row) pairs of a platform that moved, largest first."""
    moved = [
        (scenario, row)
        for scenario, rows in scenarios.items()
        for row in rows
        if row["status"] != "MISSING" and row["verdict"] != "unchanged"
    ]
    return sorted(moved, key=lambda item: -abs(item[1]["change"]))


def missing_count(scenarios: dict[str, list[dict]]) -> int:
    return sum(row["status"] == "MISSING" for rows in scenarios.values() for row in rows)


def platform_headline(scenarios: dict[str, list[dict]], threshold_pct: float) -> str:
    """One line saying what moved on a platform, or that nothing did."""
    if not scenarios:
        return "not measured on one of the two trees"

    moved = moved_rows(scenarios)
    missing = missing_count(scenarios)

    if moved:
        headline = ", ".join(
            f"{row['name']} {row['change']:+.1f}% ({scenario})"
            for scenario, row in moved[:HEADLINE_LIMIT]
        )
        if len(moved) > HEADLINE_LIMIT:
            headline += f" and {len(moved) - HEADLINE_LIMIT} more"
    else:
        headline = f"no benchmark moved by more than {threshold_pct:g}%"

    if missing:
        headline += f" ({missing} not measured)"

    return headline


def print_comparison(results: dict[str, dict[str, list[dict]]]) -> None:
    """Print a comparison table per platform and scenario."""
    for platform, scenarios in results.items():
        for scenario, rows in scenarios.items():
            print(f"\nPlatform: {platform}, scenario: {scenario}")
            print(f"{'benchmark':<32}{'baseline':>12}{'current':>12}{'change':>10}  status")
            for row in rows:
                if row["status"] == "MISSING":
                    base = "-" if row["base"] is None else f"{row['base']:.1f}"
                    cur = "-" if row["current"] is None else f"{row['current']:.1f}"
                    print(f"{row['name']:<32}{base:>12}{cur:>12}{'':>10}  MISSING")
                    continue
                print(
                    f"{row['name']:<32}{row['base']:>12.1f}{row['current']:>12.1f}"
                    f"{row['change']:>+9.1f}%  {row['status']}"
                )


def _cell(row: dict | None) -> str:
    """One table cell: the value, or both values and the change when it moved."""
    if row is None:
        return ""
    if row["status"] == "MISSING":
        base = "-" if row["base"] is None else f"{row['base']:.0f}"
        cur = "-" if row["current"] is None else f"{row['current']:.0f}"
        return f"{base} → {cur} ❔"
    if row["verdict"] == "unchanged":
        return f"{row['current']:.0f}"
    icon = "🟢" if row["verdict"] == "faster" else "🔴"
    return f"{row['base']:.0f} → {row['current']:.0f} ({row['change']:+.1f}%) {icon}"


def write_markdown(
    path: str,
    results: dict[str, dict[str, list[dict]]],
    statistic: str,
    threshold_pct: float,
) -> None:
    """Write the comparison as markdown, for a job summary or a pull request comment.

    One collapsible block per platform, open when something moved on it. Inside,
    one row per benchmark and one column per scenario, so the same operation
    under each ready queue algorithm reads across.
    """
    out: list[str] = ["### Kernel latency", ""]

    for platform, scenarios in results.items():
        headline = platform_headline(scenarios, threshold_pct)
        if not scenarios:
            out.append(f"<b>{platform}</b> — {headline}")
            out.append("")
            continue

        moved = bool(moved_rows(scenarios))
        out.append(
            f"<details{' open' if moved else ''}><summary><b>{platform}</b> — {headline}</summary>"
        )
        out.append("")

        names = sorted({row["name"] for rows in scenarios.values() for row in rows})
        by_name = {
            scenario: {row["name"]: row for row in rows} for scenario, rows in scenarios.items()
        }

        out.append(f"| benchmark ({statistic}, cycles) | " + " | ".join(scenarios) + " |")
        out.append("| --- |" + " ---: |" * len(scenarios))
        for name in names:
            cells = [_cell(by_name[scenario].get(name)) for scenario in scenarios]
            out.append(f"| `{name}` | " + " | ".join(cells) + " |")
        out.append("")
        out.append("</details>")
        out.append("")

    out.append(
        f"Each cell is the {statistic} of the benchmark in cycles of virtual time under "
        f"QEMU icount, where the cycle counter advances with executed instructions, so a "
        f"benchmark that moved is a path that executes a different number of instructions. "
        f"Changes below {threshold_pct:g}% are reported as unchanged: the two trees are "
        f"different binaries, and code alignment alone moves a short path by a few cycles."
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
            "### Kernel latency\n\n"
            f"No comparison available: {reason}\n\n"
            "This check is informational and never fails a pull request.\n"
        )
    print(f"Wrote markdown report to {path}")


def emit_annotations(results: dict[str, dict[str, list[dict]]], threshold_pct: float) -> None:
    """Emit one ``::notice::`` workflow command per platform for the Actions log."""
    for platform, scenarios in results.items():
        detail = platform_headline(scenarios, threshold_pct)
        print(f"::notice title=Kernel latency ({platform})::{platform}: {detail}")


def results_ok(results: dict[str, dict[str, list[dict]]]) -> bool:
    """True when every platform was measured twice, nothing is missing and
    nothing slowed past the tolerance."""
    return all(
        scenarios and all(row["status"] == "OK" for rows in scenarios.values() for row in rows)
        for scenarios in results.values()
    )


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
        "--scenario-prefix",
        default=DEFAULT_SCENARIO_PREFIX,
        help="Read the scenarios whose name starts with this; the rest of the "
        f"name labels the column (default: {DEFAULT_SCENARIO_PREFIX})",
    )
    parser.add_argument(
        "--statistic",
        choices=STATISTICS,
        default=DEFAULT_STATISTIC,
        help=f"Column of the percentile row to compare (default: {DEFAULT_STATISTIC})",
    )
    parser.add_argument(
        "--platform",
        metavar="NAME",
        help="Only consider this twister platform, as the full 'board/soc' name "
        "or just the board (default: every platform in the report)",
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
        help="Compare against the twister.json of another run directly",
    )
    parser.add_argument(
        "--tolerance",
        type=float,
        default=10.0,
        help="Allowed slowdown in percent before the exit status reports a "
        "regression (default: 10.0)",
    )
    parser.add_argument(
        "--threshold",
        type=float,
        default=DEFAULT_THRESHOLD_PCT,
        help="Reporting floor in percent: a benchmark that moved by less than "
        f"this is described as unchanged (default: {DEFAULT_THRESHOLD_PCT})",
    )
    parser.add_argument(
        "--markdown",
        metavar="PATH",
        help="Write the comparison as a markdown report",
    )
    parser.add_argument(
        "--annotate",
        action="store_true",
        help="Emit ::notice:: workflow command annotations summarising each platform",
    )
    parser.add_argument(
        "--exit-zero",
        action="store_true",
        help="Always exit successfully, even when a benchmark regressed",
    )
    parser.add_argument(
        "--base-dir",
        metavar="DIR",
        default=".",
        help="Restrict all files read or written to this directory tree "
        "(default: current directory)",
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
        current = extract_metrics(twister_json, args.scenario_prefix, args.statistic, args.platform)
    except MetricsError as err:
        if markdown_path:
            write_unavailable_markdown(markdown_path, str(err))
        print(err, file=sys.stderr)
        return 0 if args.exit_zero else 1

    for platform, scenarios in sorted(current.items()):
        for scenario, benchmarks in sorted(scenarios.items()):
            print(f"Platform: {platform}, scenario: {scenario}")
            for name, value in sorted(benchmarks.items()):
                print(f"  {name} = {value:.1f}")

    if args.save:
        save_path = validate_path(args.save, args.base_dir, for_write=True)
        with open(save_path, "w") as fp:
            json.dump(current, fp, indent=4, sort_keys=True)
            fp.write("\n")
        print(f"Saved baseline to {save_path}")

    if not (args.baseline or args.baseline_twister_json):
        return 0

    try:
        if args.baseline:
            path = validate_path(args.baseline, args.base_dir, for_write=False)
            baseline = load_baseline(path)
        else:
            path = validate_path(args.baseline_twister_json, args.base_dir, for_write=False)
            baseline = extract_metrics(path, args.scenario_prefix, args.statistic, args.platform)
    except (MetricsError, OSError, json.JSONDecodeError) as err:
        # A missing baseline is expected in continuous integration when the
        # reference tree failed to build. Say so and carry on.
        print(f"No baseline to compare against: {err}", file=sys.stderr)
        if markdown_path:
            write_unavailable_markdown(markdown_path, str(err))
        if args.annotate:
            print(f"::notice title=Kernel latency::{err}")
        return 0 if args.exit_zero else 1

    results = compare_all(baseline, current, args.tolerance, args.threshold)

    print_comparison(results)

    if markdown_path:
        write_markdown(markdown_path, results, args.statistic, args.threshold)

    if args.annotate:
        emit_annotations(results, args.threshold)

    if not results_ok(results):
        print("\nKernel latency regression detected.", file=sys.stderr)
        return 0 if args.exit_zero else 1

    print("\nNo kernel latency regression.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
