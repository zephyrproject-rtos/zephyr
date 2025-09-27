#!/usr/bin/env python3

# Copyright (c) 2025 Arduino SA
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import pickle

from tabulate import tabulate

KIND_TO_COL = {
    "unset": "not set",
    "default": "default",
    "assign": "assigned",
    "select": "selected",
    "imply": "implied",
}


def source_link(refpath, fn, ln):
    if refpath:
        fn = os.path.relpath(fn, refpath)

    link_fn = fn
    disp_fn = os.path.normpath(fn).replace("../", "").replace("_", "\\_")

    return f"[{disp_fn}:{ln}](<{link_fn}#L{ln}>)"


def write_markdown(trace_data, output):
    sections = {"user": [], "hidden": [], "unset": []}

    if os.name == "nt":
        # relative paths on Windows can't span drives, so don't use them
        # generated links will be absolute
        refpath = None
    else:
        refpath = os.path.dirname(os.path.abspath(output))

    for sym in trace_data:
        sym_name, sym_vis, sym_type, sym_value, sym_src, sym_loc = sym
        if sym_vis == "n":
            section = "hidden"
        elif sym_src == "unset":
            section = "unset"
        else:
            section = "user"

        if section == "user":
            sym_name = f"`{sym_name}`"
        elif section == "hidden":
            sym_name = f"`{sym_name}` (h)"

        if sym_type == "string" and sym_value is not None:
            sym_value = f'"{sym_value}"'.replace("_", "\\_")

        if isinstance(sym_loc, tuple):
            sym_loc = source_link(refpath, *sym_loc)
        elif isinstance(sym_loc, list):
            sym_loc = " || <br> ".join(f"`{loc}`" for loc in sym_loc)
            sym_loc = sym_loc.replace("|", "\\|")
        elif sym_loc is None and sym_src == "default":
            sym_loc = "_(implicit)_"

        sym_src = KIND_TO_COL[sym_src]

        sections[section].append((sym_type, sym_name, sym_value, sym_src, sym_loc))

    lines = []
    add = lines.append

    headers = ["Type", "Name", "Value", "Source", "Location"]
    colaligns = ["right", "left", "right", "center", "left"]

    add("\n## Visible symbols\n\n")
    add(
        tabulate(
            sorted(sections["user"], key=lambda x: x[1]),
            headers=headers,
            tablefmt='pipe',
            colalign=colaligns,
        )
    )

    add("\n\n## Invisible symbols\n\n")
    add(
        tabulate(
            sorted(sections["hidden"], key=lambda x: x[1]),
            headers=headers,
            tablefmt='pipe',
            colalign=colaligns,
        )
    )

    add("\n\n## Unset symbols\n\n")
    for sym_name in sorted(x[1] for x in sections["unset"]):
        add(f"    # {sym_name} is not set\n")

    with open(output, "w") as f:
        f.writelines(lines)


def main():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("dotconfig_file", help="Input merged .config file")
    parser.add_argument("output_file", help="Output Markdown file")
    parser.add_argument("kconfig_file", help="Top-level Kconfig file", nargs="?")

    args = parser.parse_args()

    with open(args.dotconfig_file + '-trace.pickle', 'rb') as f:
        trace_data = pickle.load(f)
    write_markdown(trace_data, args.output_file)


if __name__ == '__main__':
    main()
