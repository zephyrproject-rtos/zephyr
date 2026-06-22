#!/usr/bin/env python3

# Copyright (c) 2024 Basalte bv
# SPDX-License-Identifier: Apache-2.0

import sys

# A very simple script to convert ruff format output to toml
# For example:
# ruff format --check | ./scripts/ruff/gen_format_exclude.py >> .ruff-excludes.toml

if __name__ == "__main__":
    sys.stdout.write("[format]\n")
    sys.stdout.write("exclude = [\n")
    for line in sys.stdin:
        if line.startswith("Would reformat: "):
            sys.stdout.write(f'    "./{line[16:-1]}",\n')
    sys.stdout.write("]\n")
