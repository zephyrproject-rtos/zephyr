#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2021 Intel Corporation

import argparse
import os
import re

import sh
from unidiff import PatchSet

if "ZEPHYR_BASE" not in os.environ:
    exit("$ZEPHYR_BASE environment variable undefined.")

RESERVED_NAMES_SCRIPT = "/scripts/coccinelle/reserved_names.cocci"

coccinelle_scripts = [
    RESERVED_NAMES_SCRIPT,
    "/scripts/coccinelle/same_identifier.cocci",
    "/scripts/coccinelle/boolean_strict_init.cocci",
    "/scripts/coccinelle/unnamed_parameters.cocci",
    # "/scripts/coccinelle/boolean.cocci",  # Rule 14.4 - disabled (timeout)
    # "/scripts/coccinelle/identifier_length.cocci",
]

coccinelle_reserved_names_exclude_regex = [
    r"lib/libc/.*",
    r"subsys/portability/posix/.*",
    r"include/zephyr/posix/.*",
    r"tests/subsys/portability/.*",
]

# GitHub only renders the first 10 annotations of a given level per step, the
# rest are dropped silently. Summarise anything past that in a single warning.
GITHUB_ANNOTATION_LIMIT = 10

# Every rule reports as "WARNING: Violation to rule <x.y> (<what>) ...".
rule_regex = re.compile(r"\bViolation to rule\s+(\d+\.\d+)\b")


def annotate(path, line, messages):
    """
    Emit a GitHub Actions annotation so the violation shows up on the changed
    line in the pull request instead of only in the workflow log.

    https://docs.github.com/en/actions/using-workflows/workflow-commands-for-github-actions#about-workflow-commands
    """

    def _esc(msg: str) -> str:
        return msg.replace('%', '%25').replace('\n', '%0A').replace('\r', '%0D')

    body = "\n".join(m.removeprefix("WARNING: ") for m in messages)

    rule = rule_regex.search(body)
    title = f"MISRA C:2012 Rule {rule.group(1)}" if rule else "Coding guidelines"

    # Properties are comma separated, so a comma in the title would truncate it.
    title = title.replace(',', ' ')

    print(f"::error file={path},line={line},title={title}::{_esc(body)}")


def parse_coccinelle(contents: str, violations: dict):
    # Paths may contain any character other than the colon that separates them
    # from the line number, hyphens and dots included.
    reg = re.compile("([^\\s:]+\\.[ch]:[0-9]+)(:[0-9\\-]*: )(.*)")
    for line in contents.split("\n"):
        r = reg.match(line)
        if r:
            f = r.group(1)
            if f in violations:
                violations[f].append(r.group(3))
            else:
                violations[r.group(1)] = [r.group(3)]


def parse_args():
    parser = argparse.ArgumentParser(
        description="Check commits against Cocccinelle rules", allow_abbrev=False
    )
    parser.add_argument('-r', "--repository", required=False, help="Path to repository")
    parser.add_argument('-c', '--commits', default=None, help="Commit range in the form: a..b")
    parser.add_argument("-o", "--output", required=False, help="Print violation into a file")
    parser.add_argument(
        "--annotate", action="store_true", help="Print GitHub Actions-compatible annotations."
    )
    return parser.parse_args()


def main():
    args = parse_args()
    if not args.commits:
        exit("missing commit range")

    if args.repository is None:
        repository_path = os.environ['ZEPHYR_BASE']
    else:
        repository_path = args.repository

    sh_special_args = {'_tty_out': False, '_cwd': repository_path}

    # pylint does not like the 'sh' library
    # pylint: disable=too-many-function-args,unexpected-keyword-arg
    commit = sh.git("diff", args.commits, **sh_special_args)
    patch_set = PatchSet(commit)
    zephyr_base = os.getenv("ZEPHYR_BASE")
    violations = {}
    numViolations = 0
    annotations = []

    for f in patch_set:
        c_file = f.path.endswith(".c")
        h_file = f.path.endswith(".h")
        exists = os.path.exists(zephyr_base + "/" + f.path)
        if not c_file and not h_file or not exists:
            continue

        for script in coccinelle_scripts:
            skip_reserved_names = False
            if script == RESERVED_NAMES_SCRIPT:
                for path in coccinelle_reserved_names_exclude_regex:
                    if re.match(path, f.path):
                        skip_reserved_names = True
                        break

            if skip_reserved_names:
                continue

            script_path = zephyr_base + "/" + script
            print(f"Running {script} on {f.path}")
            try:
                cocci = sh.coccicheck(
                    "--mode=report",
                    "--cocci=" + script_path,
                    f.path,
                    _timeout=10,
                    **sh_special_args,
                )
                parse_coccinelle(cocci, violations)
            except sh.TimeoutException:
                print("we timed out waiting, skipping...")

        for hunk in f:
            for line in hunk:
                if line.is_added:
                    violation = f"{f.path}:{line.target_line_no}"
                    if violation in violations:
                        v_str = "\t\n".join(violations[violation])
                        out_str = f"{violation}:{v_str}"
                        numViolations += 1
                        annotations.append((f.path, line.target_line_no, violations[violation]))
                        if args.output:
                            with open(args.output, "a+") as fp:
                                fp.write(f"{out_str}\n")
                        else:
                            print(out_str)

    if args.annotate:
        for path, line_no, messages in annotations[:GITHUB_ANNOTATION_LIMIT]:
            annotate(path, line_no, messages)

        dropped = len(annotations) - GITHUB_ANNOTATION_LIMIT
        if dropped > 0:
            print(
                f"::warning title=Coding guidelines::... and {dropped} more violation(s), "
                f"only the first {GITHUB_ANNOTATION_LIMIT} are annotated"
            )

    return numViolations


if __name__ == "__main__":
    ret = main()
    exit(ret)
