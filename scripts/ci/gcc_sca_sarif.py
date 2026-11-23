#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Merge the SARIF files produced by the GCC static analyzer into one upload.

``-fdiagnostics-format=sarif-file`` makes GCC write one SARIF file per
translation unit, named after the input file and dropped in the compiler's
working directory (the CMake build directory).  A twister run therefore leaves
thousands of these scattered across the output tree, the vast majority holding
no results at all.  GitHub wants a single file, so they have to be merged.

The merge is not just concatenation; four things have to be fixed up before
code scanning will take the result:

* GCC records **absolute** paths.  GitHub resolves result locations against the
  repository root and silently drops anything it cannot map, so paths are
  rewritten relative to the source root and results outside it (SDK headers,
  west modules) are discarded rather than uploaded as unmappable noise.
* Each run embeds the full text of every artifact it touched under
  ``artifacts[].contents``, which is where nearly all of the file size comes
  from -- a single translation unit reporting two leaks weighs 66 KB.  GitHub
  rejects oversized uploads, so the embedded contents are dropped.
* The tool name varies with the input language (``GNU C17``, ``GNU C++17``).
  Code scanning keys alert identity partly on the tool, so the driver name is
  normalised to keep alerts stable across the tree and across runs.
* Shared code is recompiled by every build in the matrix, so the same finding
  reappears once per build.  Results are deduplicated on
  (rule, file, line, column, message).

The CWE taxonomy GCC already emits is preserved and merged: it is what makes
the alerts land in the dashboard with a CWE attached.

Usage:
    python3 scripts/ci/gcc_sca_sarif.py twister-out-gcc-sca -o results.sarif
"""

import argparse
import collections
import json
import os
import pathlib
import sys

import sarif_utils

# Conservative ceiling. GitHub imposes its own per-upload limits and rejects
# the whole file when they are exceeded; losing every result to a hard reject
# is worse than uploading the first N and saying so. Truncation is always
# reported, never silent.
DEFAULT_MAX_RESULTS = 5000

DRIVER_NAME = "GCC Static Analyzer"
DRIVER_URI = "https://gcc.gnu.org/onlinedocs/gcc/Static-Analyzer-Options.html"

# GCC's rule metadata is just an id and a help URI. GitHub files an alert as a
# security alert only when its rule carries the "security" tag, and ranks it by
# "security-severity", so forwarding GCC's rules verbatim would land a
# use-after-free here as an ordinary quality alert while the identical finding
# from the sanitizer scan lands as a critical security one. The severities
# below match sanitizer_sarif.py so that the two scans rank the same defect
# classes the same way.
SEVERITY = {"corruption": "9.0", "undefined": "7.0", "leak": "5.0"}

# Memory corruption: a reachable write or read outside the object's lifetime
# or bounds.
_CORRUPTION = (
    "-Wanalyzer-allocation-size",
    "-Wanalyzer-double-free",
    "-Wanalyzer-free-of-non-heap",
    "-Wanalyzer-mismatching-deallocation",
    "-Wanalyzer-out-of-bounds",
    "-Wanalyzer-overlapping-buffers",
    "-Wanalyzer-putenv-of-auto-var",
    "-Wanalyzer-stale-setjmp-buffer",
    "-Wanalyzer-tainted-allocation-size",
    "-Wanalyzer-tainted-array-index",
    "-Wanalyzer-tainted-offset",
    "-Wanalyzer-tainted-size",
    "-Wanalyzer-use-after-free",
    "-Wanalyzer-use-of-pointer-in-stale-stack-frame",
    "-Wanalyzer-write-to-const",
    "-Wanalyzer-write-to-string-literal",
    # The front end finds the same defect classes without the analyzer, and
    # ships them through the same SARIF file.
    "-Warray-bounds",
    "-Wdangling-pointer",
    "-Wfree-nonheap-object",
    "-Wreturn-local-addr",
    "-Wstringop-overflow",
    "-Wstringop-overread",
    "-Wuse-after-free",
)

# Undefined behaviour, and untrusted input reaching an operation that trusts it.
_UNDEFINED = (
    "-Wanalyzer-deref-before-check",
    "-Wanalyzer-double-fclose",
    "-Wanalyzer-exposure-through-output-file",
    "-Wanalyzer-exposure-through-uninit-copy",
    "-Wanalyzer-fd-access-mode-mismatch",
    "-Wanalyzer-fd-double-close",
    "-Wanalyzer-fd-phase-mismatch",
    "-Wanalyzer-fd-type-mismatch",
    "-Wanalyzer-fd-use-after-close",
    "-Wanalyzer-fd-use-without-check",
    "-Wanalyzer-infinite-recursion",
    "-Wanalyzer-jump-through-null",
    "-Wanalyzer-null-argument",
    "-Wanalyzer-null-dereference",
    "-Wanalyzer-possible-null-argument",
    "-Wanalyzer-possible-null-dereference",
    "-Wanalyzer-shift-count-negative",
    "-Wanalyzer-shift-count-overflow",
    "-Wanalyzer-tainted-assertion",
    "-Wanalyzer-tainted-divisor",
    "-Wanalyzer-undefined-behavior-strtok",
    "-Wanalyzer-unsafe-call-within-signal-handler",
    "-Wanalyzer-use-of-uninitialized-value",
    "-Wanalyzer-va-arg-type-mismatch",
    "-Wanalyzer-va-list-exhausted",
    "-Wanalyzer-va-list-use-after-va-end",
    "-Wmaybe-uninitialized",
    "-Wnonnull",
    "-Wuninitialized",
)

# Resources the code allocates and never gives back.
_LEAK = (
    "-Wanalyzer-fd-leak",
    "-Wanalyzer-file-leak",
    "-Wanalyzer-malloc-leak",
    "-Wanalyzer-va-list-leak",
)

RULE_CLASS = {
    rule: severity_class
    for severity_class, rules in (
        ("corruption", _CORRUPTION),
        ("undefined", _UNDEFINED),
        ("leak", _LEAK),
    )
    for rule in rules
}


def annotate_rule(rule):
    """Tag *rule* as a security rule when it describes a security defect.

    Anything not in the table keeps GCC's metadata untouched and surfaces as an
    ordinary alert: -Wanalyzer-too-complex and friends report on the analyzer
    giving up, not on a defect in the code, and tagging those as security would
    only dilute the dashboard.
    """
    # Warnings that take a level spell their rule id with the trailing "=",
    # as in "-Warray-bounds=".
    severity_class = RULE_CLASS.get(rule["id"].rstrip("="))
    if severity_class is None:
        return rule

    properties = dict(rule.get("properties", {}))
    properties["tags"] = sorted({*properties.get("tags", []), "security"})
    properties["security-severity"] = SEVERITY[severity_class]
    return dict(rule, properties=properties)


def result_key(result):
    """Identity used for deduplication across builds, and for sorting.

    Every field here is optional in SARIF -- GCC emits diagnostics with no
    ruleId, and regions without a column -- so the key is stringified to stay
    totally ordered. Comparing a missing ruleId against a present one would
    otherwise raise partway through the sort.
    """
    location = (result.get("locations") or [{}])[0]
    physical = location.get("physicalLocation", {})
    region = physical.get("region", {})
    return sarif_utils.stable_key(
        result.get("ruleId"),
        physical.get("artifactLocation", {}).get("uri"),
        region.get("startLine"),
        region.get("startColumn"),
        result.get("message", {}).get("text"),
    )


def collect(paths, source_root):
    """Merge every SARIF file under *paths*.

    Returns (results, rules, taxonomies, stats).
    """
    results = {}
    rules = {}
    taxonomies = {}
    stats = collections.Counter()

    for sarif_path in sorted(paths):
        stats["files"] += 1
        try:
            with open(sarif_path, encoding="utf-8") as handle:
                document = json.load(handle)
        except (OSError, json.JSONDecodeError) as err:
            # A build killed mid-write leaves a truncated file. One unreadable
            # translation unit must not sink the whole report.
            print(f"warning: skipping {sarif_path}: {err}", file=sys.stderr)
            stats["unreadable"] += 1
            continue

        for run in document.get("runs", []):
            driver = run.get("tool", {}).get("driver", {})
            for rule in driver.get("rules", []):
                rules.setdefault(rule["id"], rule)

            for taxonomy in run.get("taxonomies", []):
                merged = taxonomies.setdefault(taxonomy.get("name"), dict(taxonomy, taxa=[]))
                known = {taxon["id"] for taxon in merged["taxa"]}
                merged["taxa"].extend(
                    taxon for taxon in taxonomy.get("taxa", []) if taxon["id"] not in known
                )

            for result in run.get("results", []):
                stats["raw"] += 1

                locations = result.get("locations") or []
                if not locations:
                    stats["unlocated"] += 1
                    continue
                if not sarif_utils.rewrite_location(locations[0], source_root):
                    stats["external"] += 1
                    continue

                # Secondary locations are kept only when they survive the
                # rewrite. Dropping them is not cosmetic: a related location
                # with no physical location of its own, which GCC emits for
                # notes like "argument 1 of 'x' must be non-null", makes code
                # scanning reject the entire upload.
                result["locations"] = [locations[0]] + [
                    extra
                    for extra in locations[1:]
                    if sarif_utils.rewrite_location(extra, source_root)
                ]
                related = [
                    extra
                    for extra in (result.get("relatedLocations") or [])
                    if sarif_utils.rewrite_location(extra, source_root)
                ]
                if related:
                    result["relatedLocations"] = related
                else:
                    result.pop("relatedLocations", None)

                sarif_utils.prune_code_flows(result, source_root)

                key = result_key(result)
                if key in results:
                    stats["duplicate"] += 1
                    continue
                results[key] = result

    return list(results.values()), rules, taxonomies, stats


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("search_root", help="directory to scan for *.sarif files")
    parser.add_argument("-o", "--output", default="results.sarif")
    parser.add_argument(
        "--source-root",
        default=os.environ.get("ZEPHYR_BASE", os.getcwd()),
        help="repository root that result paths are made relative to",
    )
    parser.add_argument("--max-results", type=int, default=DEFAULT_MAX_RESULTS)
    args = parser.parse_args()

    source_root = pathlib.Path(args.source_root).resolve()
    sarif_files = list(pathlib.Path(args.search_root).rglob("*.sarif"))

    results, rules, taxonomies, stats = collect(sarif_files, source_root)

    # Report the worst first so that truncation, if it happens, keeps the
    # findings most likely to matter.
    severity = {"error": 0, "warning": 1, "note": 2}
    results.sort(key=lambda r: (severity.get(r.get("level"), 3), result_key(r)))

    results, dropped = sarif_utils.cap_results(results, args.max_results)

    document = sarif_utils.build_document(
        DRIVER_NAME,
        DRIVER_URI,
        [annotate_rule(rule) for rule in sorted(rules.values(), key=lambda r: r["id"])],
        results,
        list(taxonomies.values()),
    )

    sarif_utils.write_report(
        document,
        args.output,
        dropped,
        args.max_results,
        f"scanned {stats['files']} SARIF files: {stats['raw']} raw results, "
        f"{stats['duplicate']} duplicates, {stats['external']} outside the source root, "
        f"{stats['unlocated']} without a location, {stats['unreadable']} unreadable files",
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
