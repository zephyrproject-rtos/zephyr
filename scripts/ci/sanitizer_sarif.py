#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0
"""Turn sanitizer findings from a twister run into a SARIF report.

Twister has no SARIF writer, and sanitizer output is not part of its report
model: a run that trips AddressSanitizer is recorded as ``rc=1`` and nothing
more.  The diagnostics themselves survive only in the per-instance logs, so
this walks the twister output tree and reconstructs them.

Two logs matter, because the sanitizers do not agree on where to write:

    handler.log         ASan and LSan (twister sets ASAN_OPTIONS=log_path=stdout)
    handler_stderr.log  UBSan, which ignores that and writes to stderr

Three report formats are parsed:

    ASan   ==PID==ERROR: AddressSanitizer: <type> on address 0x...
           followed by one or more "    #N 0xaddr in <func> <file>:<line>"
           stacks and a SUMMARY line.
    LSan   ==PID==ERROR: LeakSanitizer: detected memory leaks
           followed by one "Direct leak"/"Indirect leak" block per leak, each
           with its own allocation stack.
    UBSan  <file>:<line>:<col>: runtime error: <description>
           a single line, with no stack unless print_stacktrace is enabled.

Unlike a static analyzer, these findings are proven reachable -- the code
actually executed and misbehaved -- so they are reported at "error" level.
Coverage is bounded by what the tests exercise, which is the opposite
trade-off to the static scans.

Usage:
    python3 scripts/ci/sanitizer_sarif.py twister-out-asan -o results.sarif
"""

import argparse
import collections
import os
import pathlib
import re
import sys

import sarif_utils

DRIVER_NAME = "Sanitizers"
DRIVER_URI = "https://github.com/google/sanitizers"

DEFAULT_MAX_RESULTS = 5000

# A stack frame with source information, e.g.
#     #1 0x8049a1f in sanleak_test_leak /path/to/main.c:8
# Frames without a resolved source file (bare addresses into libc) are left
# unmatched on purpose: they carry nothing GitHub could render.
FRAME_RE = re.compile(
    r"^\s*#(?P<depth>\d+)\s+0x[0-9a-fA-F]+\s+in\s+(?P<func>\S+)\s+"
    r"(?P<file>[^\s]+?):(?P<line>\d+)(?::(?P<col>\d+))?\s*$"
)

ASAN_ERROR_RE = re.compile(r"^==\d+==ERROR:\s+AddressSanitizer:\s+(?P<kind>[a-zA-Z0-9_-]+)")
LSAN_ERROR_RE = re.compile(r"^==\d+==ERROR:\s+LeakSanitizer:\s+(?P<desc>.+?)\s*$")
LEAK_BLOCK_RE = re.compile(
    r"^(?P<kind>Direct|Indirect) leak of (?P<bytes>\d+) byte\(s\) in "
    r"(?P<objects>\d+) object\(s\) allocated from:"
)
UBSAN_RE = re.compile(
    r"^(?P<file>[^\s]+?):(?P<line>\d+):(?P<col>\d+):\s+runtime error:\s+(?P<desc>.+?)\s*$"
)

# Hex addresses differ between runs, so they are masked out of the text used
# for deduplication. Without this the same defect looks new on every run and
# the dashboard fills with duplicates that never close.
ADDR_RE = re.compile(r"0x[0-9a-fA-F]+")

# Only mappings that are unambiguous. A wrong CWE is worse than none, so
# checks whose classification is arguable are left untagged.
ASAN_CWE = {
    "heap-buffer-overflow": "122",
    "stack-buffer-overflow": "121",
    "stack-buffer-underflow": "124",
    "global-buffer-overflow": "787",
    "heap-use-after-free": "416",
    "stack-use-after-return": "562",
    "stack-use-after-scope": "562",
    "double-free": "415",
    "alloc-dealloc-mismatch": "762",
    "SEGV": "476",
}

LSAN_CWE = "401"

UBSAN_CWE = (
    ("signed integer overflow", "190"),
    ("unsigned integer overflow", "190"),
    ("division by zero", "369"),
    ("null pointer", "476"),
    ("out of bounds", "125"),
    ("shift exponent", "1335"),
)

# GitHub ranks alerts by security-severity and only files them as security
# alerts when the rule is tagged as such. Memory corruption outranks a leak.
SEVERITY = {"corruption": "9.0", "leak": "5.0", "undefined": "7.0"}


def classify_ubsan(description):
    """Return (rule suffix, cwe) for a UBSan description."""
    lowered = description.lower()
    for needle, cwe in UBSAN_CWE:
        if needle in lowered:
            return needle.replace(" ", "-"), cwe
    return "runtime-error", None


def _frame_location(match, message=None):
    """Build a SARIF location from a stack frame match."""
    region = {"startLine": int(match.group("line"))}
    if match.group("col"):
        region["startColumn"] = int(match.group("col"))

    location = {
        "physicalLocation": {
            "artifactLocation": {"uri": match.group("file")},
            "region": region,
        }
    }
    if message:
        location["message"] = {"text": message}
    return location


def parse_stack(lines, start):
    """Collect consecutive stack frames beginning at *start*.

    Returns (frames, next_index). Frames that carry no source location end the
    run of interest but do not stop collection, so a stack that dips through
    libc and comes back is not truncated.
    """
    frames = []
    index = start
    while index < len(lines):
        line = lines[index]
        match = FRAME_RE.match(line)
        if match:
            frames.append(match)
            index += 1
            continue
        # A bare "#N 0xaddr (...)" frame has no source info; skip it but stay
        # in the stack.
        if re.match(r"^\s*#\d+\s+0x[0-9a-fA-F]+", line):
            index += 1
            continue
        break
    return frames, index


def _make_result(rule_id, level, text, frames, cwe, properties, source_root):
    """Build a SARIF result from a message and its stack frames.

    The primary location is the shallowest frame that lies inside the
    repository, not simply the shallowest frame: an ASan or LSan stack almost
    always begins inside the sanitizer runtime's own interceptor (malloc,
    free, memcpy), and anchoring an alert there would point every leak at
    libsanitizer instead of at the code that leaked. The full stack is kept as
    a code flow; sarif_utils.prune_code_flows strips the external frames from
    it afterwards.

    Returns None when no frame is in the repository, which is how findings
    entirely inside the toolchain get dropped.
    """
    primary = next(
        (
            frame
            for frame in frames
            if sarif_utils.relativize(frame.group("file"), source_root) is not None
        ),
        None,
    )
    if primary is None:
        return None

    result = {
        "ruleId": rule_id,
        "level": level,
        "message": {"text": text},
        "locations": [_frame_location(primary)],
        "properties": properties,
    }

    if len(frames) > 1:
        result["codeFlows"] = [
            {
                "threadFlows": [
                    {
                        "locations": [
                            {
                                "location": _frame_location(
                                    frame, f"#{frame.group('depth')} {frame.group('func')}"
                                )
                            }
                            for frame in frames
                        ]
                    }
                ]
            }
        ]

    if cwe:
        result["taxa"] = [{"id": cwe, "toolComponent": {"name": "CWE"}}]

    return result


def parse_log(text, instance, source_root):
    """Yield (result, cwe, severity_class) for every finding in *text*."""
    lines = text.splitlines()
    index = 0

    while index < len(lines):
        line = lines[index]

        asan = ASAN_ERROR_RE.match(line)
        if asan:
            kind = asan.group("kind")
            # The access line ("WRITE of size 1 at ...") sits between the
            # error banner and the stack; skip ahead to the frames.
            frames, index = parse_stack(lines, index + 1)
            if not frames:
                frames, index = parse_stack(lines, index + 1)
            cwe = ASAN_CWE.get(kind)
            result = _make_result(
                f"asan-{kind}",
                "error",
                f"AddressSanitizer: {kind}",
                frames,
                cwe,
                {"sanitizer": "address", "testInstance": instance},
                source_root,
            )
            if result:
                yield result, cwe, "corruption"
            continue

        if LSAN_ERROR_RE.match(line):
            index += 1
            continue

        leak = LEAK_BLOCK_RE.match(line)
        if leak:
            frames, index = parse_stack(lines, index + 1)
            kind = leak.group("kind").lower()
            result = _make_result(
                f"lsan-{kind}-leak",
                "error",
                f"LeakSanitizer: {leak.group('kind').lower()} leak of "
                f"{leak.group('bytes')} byte(s) in {leak.group('objects')} object(s)",
                frames,
                LSAN_CWE,
                {"sanitizer": "leak", "testInstance": instance},
                source_root,
            )
            if result:
                yield result, LSAN_CWE, "leak"
            continue

        ubsan = UBSAN_RE.match(line)
        if ubsan:
            suffix, cwe = classify_ubsan(ubsan.group("desc"))
            frames, index = parse_stack(lines, index + 1)
            result = {
                "ruleId": f"ubsan-{suffix}",
                "level": "error",
                "message": {"text": f"UndefinedBehaviorSanitizer: {ubsan.group('desc')}"},
                "locations": [_frame_location(ubsan)],
                "properties": {"sanitizer": "undefined", "testInstance": instance},
            }
            if cwe:
                result["taxa"] = [{"id": cwe, "toolComponent": {"name": "CWE"}}]
            if frames:
                result["codeFlows"] = [
                    {
                        "threadFlows": [
                            {
                                "locations": [
                                    {"location": _frame_location(frame)} for frame in frames
                                ]
                            }
                        ]
                    }
                ]
            yield result, cwe, "undefined"
            continue

        index += 1


def result_key(result):
    """Identity used for deduplication across test instances.

    Deliberately excludes the test instance and any hex address: the same
    defect reached by twenty tests is one defect, and its addresses differ on
    every run.
    """
    physical = result["locations"][0]["physicalLocation"]
    region = physical.get("region", {})
    return (
        result["ruleId"],
        physical["artifactLocation"]["uri"],
        region.get("startLine"),
        ADDR_RE.sub("0xADDR", result["message"]["text"]),
    )


def collect(outdir, source_root):
    """Parse every sanitizer log under *outdir*."""
    results = {}
    cwes = set()
    rule_class = {}
    stats = collections.Counter()

    logs = sorted(pathlib.Path(outdir).rglob("handler.log")) + sorted(
        pathlib.Path(outdir).rglob("handler_stderr.log")
    )

    for log_path in logs:
        stats["logs"] += 1
        try:
            text = log_path.read_text(encoding="utf-8", errors="replace")
        except OSError as err:
            print(f"warning: skipping {log_path}: {err}", file=sys.stderr)
            stats["unreadable"] += 1
            continue

        if not text:
            continue

        # The build directory name is the twister instance name, which is what
        # tells a maintainer how to reproduce the finding.
        instance = log_path.parent.name

        for result, cwe, severity_class in parse_log(text, instance, source_root):
            stats["raw"] += 1

            if not sarif_utils.rewrite_location(result["locations"][0], source_root):
                stats["external"] += 1
                continue
            sarif_utils.prune_code_flows(result, source_root)

            key = result_key(result)
            if key in results:
                stats["duplicate"] += 1
                continue

            results[key] = result
            rule_class[result["ruleId"]] = severity_class
            if cwe:
                cwes.add(cwe)

    return list(results.values()), cwes, rule_class, stats


def build_rules(rule_class):
    """Build rule metadata, tagged so GitHub files these as security alerts."""
    return [
        {
            "id": rule_id,
            "name": rule_id,
            "shortDescription": {"text": rule_id},
            "properties": {
                "tags": ["security"],
                "security-severity": SEVERITY.get(severity_class, "5.0"),
            },
        }
        for rule_id, severity_class in sorted(rule_class.items())
    ]


def main():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("outdir", help="twister output directory to scan")
    parser.add_argument("-o", "--output", default="results.sarif")
    parser.add_argument(
        "--source-root",
        default=os.environ.get("ZEPHYR_BASE", os.getcwd()),
        help="repository root that result paths are made relative to",
    )
    parser.add_argument("--max-results", type=int, default=DEFAULT_MAX_RESULTS)
    args = parser.parse_args()

    source_root = pathlib.Path(args.source_root).resolve()
    results, cwes, rule_class, stats = collect(args.outdir, source_root)

    results.sort(key=result_key)
    results, dropped = sarif_utils.cap_results(results, args.max_results)

    document = sarif_utils.build_document(
        DRIVER_NAME,
        DRIVER_URI,
        build_rules(rule_class),
        results,
        sarif_utils.make_cwe_taxonomy(cwes),
    )

    sarif_utils.write_report(
        document,
        args.output,
        dropped,
        args.max_results,
        f"scanned {stats['logs']} logs: {stats['raw']} raw findings, "
        f"{stats['duplicate']} duplicates, {stats['external']} outside the source root, "
        f"{stats['unreadable']} unreadable logs",
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
