#!/usr/bin/env python3

# Copyright (c) 2024 Basalte bv
# SPDX-License-Identifier: Apache-2.0

import json
import sys
from pathlib import Path, PurePosixPath

# A very simple script to convert ruff lint output from json to toml
# For example:
# ruff check --output-format=json | ./scripts/ruff/gen_lint_exclude.py >> .ruff-excludes.toml


class RuffRule:
    def __init__(self, code: str, url: str) -> None:
        self.code = code
        self.url = url

    def __eq__(self, other: object) -> bool:
        if not isinstance(other, type(self)):
            return NotImplemented
        return self.code.__eq__(other.code)

    def __hash__(self) -> int:
        return self.code.__hash__()


if __name__ == "__main__":
    violations = json.load(sys.stdin)
    sys.stdout.write("[lint.per-file-ignores]\n")

    rules: dict[str, set[RuffRule]] = {}
    for v in violations:
        rules.setdefault(v["filename"], set()).add(RuffRule(v["code"], v["url"]))

    for f, rs in rules.items():
        path = PurePosixPath(f)
        sys.stdout.write(f'"./{path.relative_to(Path.cwd())}" = [\n')
        for r in sorted(rs, key=lambda x: x.code):
            sys.stdout.write(f'    "{r.code}",\t# {r.url}\n'.expandtabs())
        sys.stdout.write("]\n")
