#!/usr/bin/env python3

# Copyright (c) 2025 Arduino SA
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import pickle
from pathlib import Path

from tabulate import tabulate

KIND_TO_COL = {
    "unset": "not set",
    "default": "default",
    "assign": "assigned",
    "select": "selected",
    "imply": "implied",
}


def is_cross_drive(path1: str | os.PathLike, path2: str | os.PathLike) -> bool:
    """Check if two paths are on different drives (Windows only)"""

    if os.name != 'nt':
        return False

    # handles "C:" and UNC like "\\SERVER\SHARE"
    d1 = Path(path1).drive.upper()
    d2 = Path(path2).drive.upper()

    return bool(d1 and d2 and d1 != d2)


def source_link(refpath, fn, ln, use_absolute):
    full_path = Path(fn).absolute().as_posix().replace("_", "\\_")

    if use_absolute:
        return f"{full_path}:{ln}"

    try:
        fn = os.path.relpath(fn, refpath)
    except ValueError:
        # fallback to absolute path
        return f"{full_path}:{ln}"

    link_fn = Path(fn).as_posix()
    disp_fn = link_fn.replace("../", "").replace("_", "\\_")

    return f"[{disp_fn}:{ln}](<{link_fn}#L{ln}>)"


def write_markdown(trace_data, output):
    sections = {"user": [], "hidden": [], "unset": []}

    refpath = os.path.dirname(os.path.abspath(output))

    # Check if we need to use absolute paths (cross-drive on Windows)
    use_absolute = False
    if os.name == 'nt' and is_cross_drive(refpath, __file__):
        use_absolute = True

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
            sym_loc = source_link(refpath, *sym_loc, use_absolute)
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
    print(f"\nTraceconfig generated to: {args.output_file}")


if __name__ == '__main__':
    main()
