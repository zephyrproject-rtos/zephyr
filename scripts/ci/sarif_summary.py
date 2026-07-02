#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Generate a GitHub Actions step summary from a SARIF file.

Reads the SARIF file produced by the scan tools and writes a Markdown
table to $GITHUB_STEP_SUMMARY.  Also emits one ::notice:: workflow
annotation per triggered rule so each rule appears in the Actions log
with its violation count and description.

Usage:
    python3 scripts/ci/sarif_summary.py [results.sarif]
"""

import argparse
import collections
import json
import os
import sys


def _tag_order(tag):
    return {"mandatory": 0, "required": 1, "advisory": 2}.get(tag, 3)


def parse_sarif(path):
    """Return (counts, rule_meta, file_counts) from *path*.

    counts      -- Counter mapping ruleId -> total violation count
    rule_meta   -- dict mapping ruleId -> {desc, tag}
    file_counts -- Counter mapping file URI -> total violation count
    """
    with open(path, encoding="utf-8") as fh:
        sarif = json.load(fh)

    counts = collections.Counter()
    rule_meta = {}
    file_counts = collections.Counter()

    for run in sarif.get("runs", []):
        for rule in run.get("tool", {}).get("driver", {}).get("rules", []):
            rid = rule.get("id", "")
            tags = rule.get("properties", {}).get("tags", [])
            # Pick the most restrictive classification tag present.
            tag = ""
            for candidate in ("mandatory", "required", "advisory"):
                if candidate in tags:
                    tag = candidate
                    break
            rule_meta[rid] = {
                "desc": rule.get("fullDescription", {}).get("text", rid),
                "tag": tag,
            }

        for result in run.get("results", []):
            rid = result.get("ruleId", "unknown")
            counts[rid] += 1
            for loc in result.get("locations", []):
                uri = loc.get("physicalLocation", {}).get("artifactLocation", {}).get("uri", "")
                if uri:
                    file_counts[uri] += 1

    return counts, rule_meta, file_counts


def write_file_summary(file_counts, out):
    """Write a Markdown table of violation counts per file."""
    n_files = len(file_counts)
    out.write("## :file_folder: Violations by File\n\n")
    out.write(f"> **Files with violations:** {n_files:,}\n\n")
    out.write("| # | File | Violations |\n")
    out.write("|--:|------|----------:|\n")
    for idx, (uri, cnt) in enumerate(sorted(file_counts.items(), key=lambda kv: -kv[1]), 1):
        out.write(f"| {idx} | `{uri}` | {cnt:,} |\n")


def write_summary(counts, rule_meta, file_counts, out):
    total = sum(counts.values())
    n_rules = len(counts)

    out.write("## :mag: Scan Results\n\n")
    out.write(f"> **Total violations:** {total:,} &nbsp;|&nbsp; **Rules triggered:** {n_rules}\n\n")

    # Sort by tag priority then by count (descending within same tag).
    sorted_rules = sorted(
        counts.items(),
        key=lambda kv: (_tag_order(rule_meta.get(kv[0], {}).get("tag", "")), -kv[1]),
    )

    out.write("| # | Rule ID | Classification | Violations | Description |\n")
    out.write("|--:|---------|---------------|----------:|-------------|\n")
    for idx, (rid, cnt) in enumerate(sorted_rules, 1):
        m = rule_meta.get(rid, {})
        tag = m.get("tag", "")
        desc = m.get("desc", "")
        out.write(f"| {idx} | `{rid}` | {tag} | {cnt:,} | {desc} |\n")

    out.write("\n")
    write_file_summary(file_counts, out)


def _escape_data(value):
    """Escape a workflow command message so it cannot break the format."""
    return str(value).replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")


def _escape_property(value):
    """Escape a workflow command property value (also escapes ':' and ',')."""
    return _escape_data(value).replace(":", "%3A").replace(",", "%2C")


def emit_annotations(counts, rule_meta):
    """Emit one ``::notice::`` annotation per rule for the Actions log."""
    for rid, cnt in sorted(counts.items(), key=lambda kv: -kv[1]):
        m = rule_meta.get(rid, {})
        desc = m.get("desc", rid)
        tag = m.get("tag", "")
        prefix = f"[{tag}] " if tag else ""
        title = _escape_property(rid)
        message = _escape_data(f"{prefix}{desc} - {cnt} violation(s)")
        print(f"::notice title={title}::{message}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    ap.add_argument(
        "sarif",
        nargs="?",
        default="results.sarif",
        help="Path to the SARIF file (default: results.sarif)",
    )
    ap.add_argument(
        "--annotations",
        action=argparse.BooleanOptionalAction,
        default=True,
        help="Emit ::notice:: workflow command annotations",
    )
    args = ap.parse_args()

    counts, rule_meta, file_counts = parse_sarif(args.sarif)

    summary_file = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary_file:
        with open(summary_file, "a", encoding="utf-8") as fh:
            write_summary(counts, rule_meta, file_counts, fh)
    else:
        write_summary(counts, rule_meta, file_counts, sys.stdout)

    if args.annotations:
        emit_annotations(counts, rule_meta)


if __name__ == "__main__":
    main()
