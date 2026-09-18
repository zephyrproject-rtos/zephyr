#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Generate the Mach-O iterable section bounds an image references.

ld64 has no section bound symbols of its own: a reference to
section$start$<segment>$<section> makes it materialise an output section, and it
runs out of section indexes after 127 of them. Emitting a bound for every
iterable section Zephyr has is well past that, so the bounds are taken from the
symbols the built image leaves undefined, which is exactly the set it uses.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

# A bound of the section <name> is _<name>_list_start and so on, and Mach-O
# prefixes a symbol with one more underscore
BOUND_RE = re.compile(r"^__(?P<name>.+)_(?P<bound>list_start|list_end|ext_end)$")

BOUND_KIND = {"list_start": "start", "list_end": "end", "ext_end": "end"}


def read_mapping(path):
    """Return the logical to Mach-O section name mapping."""
    mapping = {}
    for line in Path(path).read_text(encoding="utf-8").splitlines():
        if not line.strip():
            continue
        name, short_name = line.split()
        mapping[name] = short_name

    return mapping


def undefined_bounds(nm, image):
    """Return the section bounds the image leaves undefined, as (name, bound)."""
    result = subprocess.run([str(nm), "-u", str(image)], check=True, capture_output=True, text=True)
    bounds = []
    for line in result.stdout.splitlines():
        match = BOUND_RE.fullmatch(line.strip())
        if match is not None:
            bounds.append((match.group("name"), match.group("bound")))

    return sorted(set(bounds))


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__, allow_abbrev=False)
    parser.add_argument("--image", required=True, type=Path)
    parser.add_argument("--mapping", required=True, type=Path)
    parser.add_argument("--nm", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    return parser.parse_args()


def main():
    args = parse_args()
    mapping = read_mapping(args.mapping)
    bounds = undefined_bounds(args.nm, args.image)

    missing = sorted({name for name, _ in bounds} - mapping.keys())
    if missing:
        sys.exit(
            f"{args.image}: no Mach-O section is mapped for {', '.join(missing)}. "
            "The section is not one gen_macho_iter_sections.py found in the tree."
        )

    lines = [
        "/* Generated file. Do not edit. */",
        "#ifdef __APPLE__",
        "#define MACHO_ALIAS(sym, target) \\",
        '\t__asm__(".globl _" #sym "\\n_" #sym " = " target)',
        "",
    ]
    for name, bound in bounds:
        target = f"section${BOUND_KIND[bound]}$__DATA${mapping[name]}"
        lines.append(f'MACHO_ALIAS(_{name}_{bound}, "{target}");')
    lines.extend(["", "#endif /* __APPLE__ */", ""])

    args.output.write_text("\n".join(lines), encoding="utf-8")


if __name__ == "__main__":
    main()
