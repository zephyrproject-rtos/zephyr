#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Shared helpers for turning scanner output into a SARIF upload.

Every scanner wired into the code scanning dashboard hits the same handful of
problems, so they are solved once here rather than per tool:

* Findings carry absolute paths. GitHub resolves result locations against the
  repository root and silently drops what it cannot map, so paths have to be
  made relative and results outside the checkout discarded.
* Stack traces and analyzer execution paths run through code that is not in
  this repository (toolchain headers, sanitizer runtimes, libc). Those steps
  have to go, without throwing away the frames that are in the tree.
* The same finding is reported once per build, because a matrix recompiles or
  re-executes shared code many times.

Used by gcc_sca_sarif.py and sanitizer_sarif.py.
"""

import json
import os
import pathlib
import sys

CWE_HELP = "https://cwe.mitre.org/data/definitions/{}.html"


def relativize(uri, source_root):
    """Return *uri* relative to *source_root*, or None if it falls outside.

    Handles the plain absolute paths scanners emit as well as ``file://``
    URIs. Symlinks are resolved on both sides so a build under a symlinked
    path still maps onto the checkout. A path that is already relative is
    assumed to be relative to the source root and returned unchanged, which
    is what makes a second merge pass over already-processed files a no-op.
    """
    if not uri:
        return None

    path = uri[len("file://") :] if uri.startswith("file://") else uri
    if not os.path.isabs(path):
        # Sanitizer runtimes report their own frames with paths relative to
        # wherever the toolchain was built, e.g.
        # "../../../../../src/libsanitizer/asan/asan_malloc_linux.cpp".
        # Those escape the source root and must not be mistaken for in-tree
        # paths just because they are not absolute.
        if os.path.normpath(path).startswith(".."):
            return None
        return path

    try:
        resolved = pathlib.Path(path).resolve()
    except OSError:
        return None

    try:
        return resolved.relative_to(source_root).as_posix()
    except ValueError:
        return None


def rewrite_location(location, source_root):
    """Rewrite one SARIF location in place.

    Returns False when the location cannot be reported, either because it
    lies outside the source root or because it has no physical location at
    all. Callers must drop what this rejects.

    The second case is not hypothetical: GCC annotates a diagnostic with
    message-only related locations ("argument 1 of '__builtin_strlen' must be
    non-null") and with code flow steps carrying nothing but a logical
    location ("looping back..." against a function name). Both are valid
    SARIF, and both make code scanning reject the entire upload with
    "expected physical location".
    """
    artifact = location.get("physicalLocation", {}).get("artifactLocation")
    if artifact is None:
        return False

    rel = relativize(artifact.get("uri"), source_root)
    if rel is None:
        return False

    artifact["uri"] = rel
    # An absolute uriBaseId would send GitHub looking outside the repository.
    artifact.pop("uriBaseId", None)
    return True


def prune_code_flows(result, source_root):
    """Rewrite code flow steps, dropping those outside the source root.

    The execution path is the most useful part of both an analyzer report and
    a sanitizer stack trace, so it is kept, but steps through code GitHub
    cannot render are removed. A flow left with no steps is dropped rather
    than uploaded empty, which SARIF forbids.
    """
    kept_flows = []
    for flow in result.get("codeFlows", []):
        kept_threads = []
        for thread in flow.get("threadFlows", []):
            steps = [
                step
                for step in thread.get("locations", [])
                if rewrite_location(step.get("location", {}), source_root)
            ]
            if steps:
                thread["locations"] = steps
                kept_threads.append(thread)
        if kept_threads:
            flow["threadFlows"] = kept_threads
            kept_flows.append(flow)

    if kept_flows:
        result["codeFlows"] = kept_flows
    else:
        result.pop("codeFlows", None)


def stable_key(*values):
    """Build a totally ordered key from optional SARIF fields.

    Almost everything in SARIF is optional, so a key assembled from raw field
    values can mix strings with None and blow up the moment two results are
    compared -- a rule without a ruleId next to one with it, for instance.
    Stringifying everything keeps the key usable both for sorting and as a
    deduplication identity.
    """
    return tuple("" if value is None else str(value) for value in values)


def count_unreportable(document):
    """Return how many locations in *document* lack a physical location.

    Code scanning rejects the whole upload when it finds one, so this is
    checked before writing rather than discovered after a job has already
    spent an hour building.
    """
    bad = 0

    def check(location):
        nonlocal bad
        if "physicalLocation" not in location:
            bad += 1

    for run in document.get("runs", []):
        for result in run.get("results", []):
            for location in result.get("locations", []):
                check(location)
            for location in result.get("relatedLocations", []):
                check(location)
            for flow in result.get("codeFlows", []):
                for thread in flow.get("threadFlows", []):
                    for step in thread.get("locations", []):
                        check(step.get("location", {}))
    return bad


def make_cwe_taxonomy(cwe_ids):
    """Build the CWE taxonomy block for the given set of CWE ids."""
    if not cwe_ids:
        return []
    return [
        {
            "name": "CWE",
            "organization": "MITRE",
            "shortDescription": {"text": "The MITRE Common Weakness Enumeration"},
            "taxa": [{"id": cwe, "helpUri": CWE_HELP.format(cwe)} for cwe in sorted(cwe_ids)],
        }
    ]


def build_document(driver_name, driver_uri, rules, results, taxonomies):
    """Assemble a single-run SARIF 2.1.0 document."""
    return {
        "$schema": "https://json.schemastore.org/sarif-2.1.0.json",
        "version": "2.1.0",
        "runs": [
            {
                "tool": {
                    "driver": {
                        "name": driver_name,
                        "informationUri": driver_uri,
                        "rules": rules,
                    }
                },
                "taxonomies": taxonomies,
                "results": results,
            }
        ],
    }


def cap_results(results, max_results):
    """Trim *results* to *max_results*, returning (results, dropped).

    Callers sort by severity first so that what survives is what matters most.
    """
    if len(results) <= max_results:
        return results, 0
    return results[:max_results], len(results) - max_results


def write_report(document, output, dropped, max_results, stats_line):
    """Write *document* to *output* and report what happened."""
    unreportable = count_unreportable(document)
    if unreportable:
        # Fail here rather than let the upload be rejected wholesale later.
        print(
            f"::error title=Malformed SARIF::{unreportable} locations have no "
            "physicalLocation; code scanning would reject this upload",
            file=sys.stderr,
        )
        raise SystemExit(1)

    with open(output, "w", encoding="utf-8") as handle:
        json.dump(document, handle)

    run = document["runs"][0]
    print(stats_line)
    print(
        f"wrote {len(run['results'])} results and "
        f"{len(run['tool']['driver']['rules'])} rules to {output}"
    )

    if dropped:
        # Surface truncation in the Actions log; a capped report that looks
        # complete is worse than one that says what it dropped.
        print(
            f"::warning title=Results truncated::dropped {dropped} results over the "
            f"{max_results} cap; the uploaded report is incomplete",
            file=sys.stderr,
        )
