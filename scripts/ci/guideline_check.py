#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
# Copyright (c) 2021 Intel Corporation

import os
import sh
import argparse
import re
from unidiff import PatchSet

if "ZEPHYR_BASE" not in os.environ:
    exit("$ZEPHYR_BASE environment variable undefined.")

repository_path = os.environ['ZEPHYR_BASE']

sh_special_args = {
    '_tty_out': False,
    '_cwd': repository_path
}

coccinelle_scripts = ["/scripts/coccinelle/reserved_names.cocci",
                      "/scripts/coccinelle/same_identifier.cocci",
                      #"/scripts/coccinelle/identifier_length.cocci",
                      ]


def parse_coccinelle(contents: str, violations: dict):
    reg = re.compile("([a-zA-Z0-9_/]*\\.[ch]:[0-9]*)(:[0-9\\-]*: )(.*)")
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
        description="Check if change requires full twister")
    parser.add_argument('-c', '--commits', default=None,
                        help="Commit range in the form: a..b")
    parser.add_argument("-o", "--output", required=False,
                        help="Print violation into a file")
    return parser.parse_args()


def main():
    args = parse_args()
    if not args.commits:
        exit("missing commit range")

    # pylint does not like the 'sh' library
    # pylint: disable=too-many-function-args,unexpected-keyword-arg
    commit = sh.git("diff", args.commits, **sh_special_args)
    patch_set = PatchSet(commit)
    zephyr_base = os.getenv("ZEPHYR_BASE")
    violations = {}
    numViolations = 0

    for f in patch_set:
        if not f.path.endswith(".c") and not f.path.endswith(".h") or not os.path.exists(zephyr_base + "/" + f.path):
            continue

        for script in coccinelle_scripts:
            script_path = os.getenv("ZEPHYR_BASE") + "/" + script
            print(f"Running {script} on {f.path}")
            try:
                cocci = sh.coccicheck(
                    "--mode=report",
                    "--cocci=" +
                    script_path,
                    f.path,
                    _timeout=10,
                    **sh_special_args)
                parse_coccinelle(cocci, violations)
            except sh.TimeoutException:
                print("we timed out waiting, skipping...")

        for hunk in f:
            for line in hunk:
                if line.is_added:
                    violation = "{}:{}".format(f.path, line.target_line_no)
                    if violation in violations:
                        numViolations += 1
                        if args.output:
                            with open(args.output, "a+") as fp:
                                fp.write("{}:{}\n".format(
                                    violation, "\t\n".join(
                                        violations[violation])))
                        else:
                            print(
                                "{}:{}".format(
                                    violation, "\t\n".join(
                                        violations[violation])))

    return numViolations


if __name__ == "__main__":
    ret = main()
    exit(ret)
