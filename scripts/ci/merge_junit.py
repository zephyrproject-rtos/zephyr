#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2026 Intel Corporation
"""Fast streaming merge of many JUnit XML files into one.

Unlike ``junitparser merge``, which builds a full object model of every input
file in memory before writing, this streams each file with ``iterparse`` and
copies ``<testsuite>`` elements straight to the output, keeping memory roughly
constant regardless of the number/size of inputs.

Usage:
    merge_junit.py OUTPUT GLOB [GLOB ...]

Example:
    merge_junit.py junit.xml 'art/*/twister.xml' 'art/*/*/twister.xml'
"""

import contextlib
import glob
import sys

from lxml import etree


def iter_files(patterns):
    seen = set()
    for pat in patterns:
        for path in glob.iglob(pat, recursive=True):
            if path not in seen:
                seen.add(path)
                yield path


def merge(output, patterns):
    totals = {"tests": 0, "failures": 0, "errors": 0, "skipped": 0}
    total_time = 0.0
    n_files = 0
    n_suites = 0

    # Stream <testsuite> elements into a body file, accumulating root totals.
    body_path = output + ".body.tmp"
    with open(body_path, "wb") as body:
        for path in iter_files(patterns):
            n_files += 1
            try:
                context = etree.iterparse(path, events=("end",), tag="testsuite")
                for _, elem in context:
                    n_suites += 1
                    for key in totals:
                        totals[key] += int(elem.get(key, 0) or 0)
                    with contextlib.suppress(ValueError):
                        total_time += float(elem.get("time", 0) or 0)
                    body.write(etree.tostring(elem))
                    # Free the subtree and any preceding siblings.
                    elem.clear()
                    while elem.getprevious() is not None:
                        del elem.getparent()[0]
                del context
            except etree.XMLSyntaxError as exc:
                sys.stderr.write(f"WARNING: skipping malformed {path}: {exc}\n")

    # Assemble final file: header with aggregated totals + body + footer.
    with open(output, "wb") as out:
        attrs = (
            f'tests="{totals["tests"]}" failures="{totals["failures"]}" '
            f'errors="{totals["errors"]}" skipped="{totals["skipped"]}" '
            f'time="{total_time:.2f}"'
        )
        out.write(b'<?xml version="1.0" encoding="utf-8"?>\n')
        out.write(f"<testsuites {attrs}>\n".encode())
        with open(body_path, "rb") as body:
            while True:
                chunk = body.read(1 << 20)
                if not chunk:
                    break
                out.write(chunk)
        out.write(b"</testsuites>\n")

    import os

    os.remove(body_path)
    sys.stderr.write(
        f"Merged {n_suites} testsuites from {n_files} files -> {output} "
        f"({totals['tests']} tests, {totals['failures']} failures, "
        f"{totals['errors']} errors, {totals['skipped']} skipped)\n"
    )


def main(argv):
    if len(argv) < 3:
        sys.stderr.write(__doc__)
        return 2
    merge(argv[1], argv[2:])
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
