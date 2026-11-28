#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Merge the SARIF files produced by the Clang Static Analyzer into one upload.

With ``CLANG_SCA_OPTS=--sarif``, ``analyze-build`` writes one ``result-*.sarif``
file per translation unit under ``<build>/sca/clang/scan-build-*/``, next to a
``results-merged.sarif`` that only concatenates them. The latter is skipped and
the per translation unit files are merged here, across every build of a twister
run, so that the same finding reported by many builds is uploaded once.

Path rewriting, deduplication and truncation work as in gcc_sca_sarif.py. On
top of that, Clang's output needs these fixes before code scanning accepts it:

* Locations refer to the run's ``artifacts`` table and results to its ``rules``
  table by index. The merged run rebuilds both tables, so the indices are
  dropped and only the URI and rule id are kept.
* Path steps inside macro expansions carry ``endLine: 0``, which the SARIF
  schema rejects. Such end positions are dropped.
* URIs are percent-encoded ``file://`` URIs.
* Generated sources and headers in the build tree have no counterpart in the
  repository, even when the build tree sits inside the checkout. Locations
  there are dropped like those outside the source root.
* Clang tags its rules with neither a CWE nor a security severity. Both are
  added for the checkers that find security defects, so that the alerts are
  filed and ranked as security alerts.

Usage:
    python3 scripts/ci/clang_sca_sarif.py twister-out-clang-sca -o results.sarif
"""

import argparse
import collections
import functools
import json
import os
import pathlib
import sys
import urllib.parse

import sarif_utils

# See gcc_sca_sarif.py.
DEFAULT_MAX_RESULTS = 5000

DRIVER_NAME = "Clang Static Analyzer"
DRIVER_URI = "https://clang.llvm.org/docs/analyzer/checkers.html"

# analyze-build's own concatenation of the per translation unit files.
ANALYZE_BUILD_MERGED = "results-merged.sarif"

# Same ranking as gcc_sca_sarif.py and sanitizer_sarif.py.
SEVERITY = {"corruption": "9.0", "undefined": "7.0", "leak": "5.0"}

# Checker -> (severity class, CWE). Only unambiguous CWEs are given. Checkers
# that are off by default, or alpha in some LLVM releases, are listed so that
# enabling them through CLANG_SCA_OPTS ranks them like the rest. Unlisted
# checkers, such as deadcode.DeadStores and security.insecureAPI.*, report on
# code quality and keep Clang's metadata.
CHECKERS = {
    # Memory corruption. unix.Malloc and cplusplus.NewDelete report double
    # frees and uses after free under one id, hence no CWE.
    "alpha.security.ArrayBoundV2": ("corruption", "119"),
    "alpha.security.ReturnPtrRange": ("corruption", "466"),
    "alpha.unix.cstring.BufferOverlap": ("corruption", None),
    "alpha.unix.cstring.OutOfBounds": ("corruption", "119"),
    "core.StackAddressEscape": ("corruption", "562"),
    "cplusplus.NewDelete": ("corruption", None),
    "optin.taint.TaintedAlloc": ("corruption", "789"),
    "security.ArrayBound": ("corruption", "119"),
    "security.PutenvStackArray": ("corruption", None),
    "unix.Malloc": ("corruption", None),
    "unix.MismatchedDeallocator": ("corruption", "762"),
    "unix.cstring.BadSizeArg": ("corruption", None),
    # Undefined behaviour, and untrusted input reaching an operation that
    # trusts it.
    "alpha.security.taint.GenericTaint": ("undefined", None),
    "alpha.unix.cstring.NotNullTerminated": ("undefined", None),
    "alpha.unix.cstring.UninitializedRead": ("undefined", None),
    "core.BitwiseShift": ("undefined", "1335"),
    "core.CallAndMessage": ("undefined", None),
    "core.DivideZero": ("undefined", "369"),
    "core.NonNullParamChecker": ("undefined", "476"),
    "core.NullDereference": ("undefined", "476"),
    "core.UndefinedBinaryOperatorResult": ("undefined", None),
    "core.VLASize": ("undefined", None),
    "core.uninitialized.ArraySubscript": ("undefined", "457"),
    "core.uninitialized.Assign": ("undefined", "457"),
    "core.uninitialized.Branch": ("undefined", "457"),
    "core.uninitialized.CapturedBlockVariable": ("undefined", "457"),
    "core.uninitialized.NewArraySize": ("undefined", "457"),
    "core.uninitialized.UndefReturn": ("undefined", "457"),
    "optin.taint.GenericTaint": ("undefined", None),
    "optin.taint.TaintedDiv": ("undefined", "369"),
    "unix.StdCLibraryFunctions": ("undefined", None),
    "unix.cstring.NotNullTerminated": ("undefined", None),
    "unix.cstring.NullArg": ("undefined", "476"),
    "valist.Uninitialized": ("undefined", None),
    # Resources the code allocates and never gives back.
    "cplusplus.NewDeleteLeaks": ("leak", "401"),
}


def annotate_rule(rule):
    """Tag *rule* as a security rule when its checker finds security defects."""
    severity_class = CHECKERS.get(rule["id"], (None, None))[0]
    if severity_class is None:
        return rule

    properties = dict(rule.get("properties", {}))
    properties["tags"] = sorted({*properties.get("tags", []), "security"})
    properties["security-severity"] = SEVERITY[severity_class]
    return dict(rule, properties=properties)


def iter_locations(result):
    """Yield every location of *result*, code flow steps included."""
    yield from result.get("locations") or []
    yield from result.get("relatedLocations") or []
    for flow in result.get("codeFlows") or []:
        for thread in flow.get("threadFlows") or []:
            for step in thread.get("locations") or []:
                if "location" in step:
                    yield step["location"]


@functools.cache
def in_build_tree(path, build_root):
    """Return True if the absolute *path* lies under *build_root*."""
    return build_root is not None and pathlib.Path(path).resolve().is_relative_to(build_root)


def normalize_location(location, build_root):
    """Undo what keeps a Clang location from surviving the merge.

    Idempotent. A location in the build tree loses its URI, which makes
    sarif_utils.rewrite_location() reject it like one outside the source root.
    """
    physical = location.get("physicalLocation", {})

    region = physical.get("region")
    if region and region.get("endLine", 1) < region.get("startLine", 1):
        region.pop("endLine", None)
        region.pop("endColumn", None)

    artifact = physical.get("artifactLocation")
    if artifact is None:
        return
    artifact.pop("index", None)

    uri = artifact.get("uri")
    if not uri:
        return
    if uri.startswith("file://"):
        uri = urllib.parse.unquote(uri[len("file://") :])
        artifact["uri"] = uri
    if os.path.isabs(uri) and in_build_tree(uri, build_root):
        del artifact["uri"]


def result_key(result):
    """Identity used for deduplication across builds, and for sorting."""
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


def collect(paths, source_root, build_root):
    """Merge every SARIF file in *paths*.

    Returns (results, rules, cwes, stats).
    """
    results = {}
    rules = {}
    cwes = set()
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
            for rule in run.get("tool", {}).get("driver", {}).get("rules", []):
                rules.setdefault(rule["id"], rule)

            for result in run.get("results", []):
                stats["raw"] += 1

                locations = result.get("locations") or []
                if not locations:
                    stats["unlocated"] += 1
                    continue
                normalize_location(locations[0], build_root)
                if not sarif_utils.rewrite_location(locations[0], source_root):
                    stats["external"] += 1
                    continue

                # Every build re-analyzes the shared code, so most results are
                # duplicates. Drop them before walking their execution paths.
                key = result_key(result)
                if key in results:
                    stats["duplicate"] += 1
                    continue

                for location in iter_locations(result):
                    normalize_location(location, build_root)
                result.pop("ruleIndex", None)

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

                cwe = CHECKERS.get(result.get("ruleId"), (None, None))[1]
                if cwe:
                    result["taxa"] = [{"id": cwe, "toolComponent": {"name": "CWE"}}]
                    cwes.add(cwe)
                results[key] = result

    return list(results.values()), rules, cwes, stats


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument(
        "search_root", help="directory to scan for *.sarif files, usually the twister output"
    )
    parser.add_argument("-o", "--output", default="results.sarif")
    parser.add_argument(
        "--source-root",
        default=os.environ.get("ZEPHYR_BASE", os.getcwd()),
        help="repository root that result paths are made relative to",
    )
    parser.add_argument("--max-results", type=int, default=DEFAULT_MAX_RESULTS)
    args = parser.parse_args()

    source_root = pathlib.Path(args.source_root).resolve()
    search_root = pathlib.Path(args.search_root).resolve()
    sarif_files = [
        path for path in search_root.rglob("*.sarif") if path.name != ANALYZE_BUILD_MERGED
    ]

    # Generated files only need filtering out when the build tree is inside
    # the checkout; outside it, path rewriting already drops them.
    build_root = None
    if search_root != source_root and search_root.is_relative_to(source_root):
        build_root = search_root

    results, rules, cwes, stats = collect(sarif_files, source_root, build_root)

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
        sarif_utils.make_cwe_taxonomy(cwes),
    )

    sarif_utils.write_report(
        document,
        args.output,
        dropped,
        args.max_results,
        f"scanned {stats['files']} SARIF files: {stats['raw']} raw results, "
        f"{stats['duplicate']} duplicates, {stats['external']} outside the source root "
        f"or generated, {stats['unlocated']} without a location, "
        f"{stats['unreadable']} unreadable files",
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
