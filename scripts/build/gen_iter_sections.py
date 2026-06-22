#!/usr/bin/env python3
#
# Copyright (c) 2023 Bjarki Arge Andreasen
#
# SPDX-License-Identifier: Apache-2.0
"""
Script to generate iterable sections from JSON encoded dictionary containing lists of items.

Supports API class inheritance: when a child API struct embeds a parent API struct as its
first member (declared via DEVICE_API_EXTENDS), the child's linker section is nested inside
the parent's address range. This allows DEVICE_API_IS(parent_class, dev) to return true for
devices with a child API. Multi-level inheritance is supported.

All device API entries are placed in a single output section. Per-class start/end symbols
and _ext_end symbols are used for runtime range checks and iteration.
"""

import argparse
import json


def parse_tagged_items(filepath: str, tag: str):
    """Parse tagged items from JSON. Each entry is either a string or an object
    with "name" and "extends" keys.

    Returns:
        items: list of all item names (strings)
        parent_children: dict mapping parent item to sorted list of child items
        children: set of items that are children
    """
    with open(filepath) as fp:
        raw = json.load(fp)[tag]

    items = []
    parent_children = {}
    children = set()

    for entry in raw:
        if isinstance(entry, str):
            items.append(entry)
        else:
            name = entry["name"]
            parent = entry["extends"]
            items.append(name)
            parent_children.setdefault(parent, []).append(name)
            children.add(name)

    # Sort children for deterministic output
    for parent in parent_children:
        parent_children[parent].sort()

    return items, parent_children, children


def walk_tree_ld(fp, item, parent_children, indent="\t"):
    """Depth-first walk emitting Z_LINK_ITERABLE + _ext_end for item and descendants."""
    fp.write(f"{indent}Z_LINK_ITERABLE({item});\n")
    for child in parent_children.get(item, []):
        walk_tree_ld(fp, child, parent_children, indent)
    fp.write(f"{indent}PLACE_SYMBOL_HERE(_{item}_ext_end);\n")


def walk_tree_cmake(fp, item, parent_children, section_name, prio):
    """Depth-first walk emitting section_settings entries for item and descendants.

    Returns the next available prio value.
    """
    # This item's input entries
    fp.write(
        f'list(APPEND section_settings "{{SECTION\\;{section_name}\\;'
        + 'SORT\\;NAME\\;'
        + 'KEEP\\;TRUE\\;'
        + f'INPUT\\;._{item}.static.*\\;'
        + f'SYMBOLS\\;_{item}_list_start\\;_{item}_list_end\\;'
        + f'PRIO\\;{prio}}}")\n'
    )
    prio += 1

    # Recurse into children
    for child in parent_children.get(item, []):
        prio = walk_tree_cmake(fp, child, parent_children, section_name, prio)

    # _ext_end symbol after all descendants
    fp.write(
        f'list(APPEND section_settings "{{SECTION\\;{section_name}\\;'
        + f'SYMBOLS\\;_{item}_ext_end\\;'
        + f'PRIO\\;{prio}}}")\n'
    )
    prio += 1

    return prio


def gen_ld(filepath: str, items: list, alignment: str, parent_children: dict, children: set):
    with open(filepath, "w") as fp:
        fp.write(f"SECTION_PROLOGUE(device_api_area,,SUBALIGN({alignment}))\n")
        fp.write("{\n")
        for item in items:
            if item in children:
                continue
            walk_tree_ld(fp, item, parent_children)
        fp.write("} GROUP_ROM_LINK_IN(RAMABLE_REGION, ROMABLE_REGION)\n")


def gen_cmake(filepath: str, items: list, alignment: str, parent_children: dict, children: set):
    with open(filepath, "w") as fp:
        section_name = "device_api_area"

        fp.write(
            f'list(APPEND sections "{{NAME\\;{section_name}\\;'
            + 'GROUP\\;RODATA_REGION\\;'
            + f'SUBALIGN\\;{alignment}\\;'
            + 'NOINPUT\\;TRUE}")\n'
        )

        prio = 100
        for item in items:
            if item in children:
                continue
            prio = walk_tree_cmake(fp, item, parent_children, section_name, prio)

        fp.write('set(DEVICE_API_SECTIONS         "${sections}" CACHE INTERNAL "")\n')
        fp.write('set(DEVICE_API_SECTION_SETTINGS "${section_settings}" CACHE INTERNAL "")\n')


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
        allow_abbrev=False,
    )

    parser.add_argument("-i", "--input", required=True, help="Path to input list of tags")
    parser.add_argument("-a", "--alignment", required=True, help="Iterable section alignment")
    parser.add_argument("-t", "--tag", required=True, help="Tag to generate iterable sections for")
    parser.add_argument("-l", "--ld-output", required=True, help="Path to output linker file")
    parser.add_argument(
        "-c", "--cmake-output", required=True, help="Path to CMake linker script inclusion file"
    )

    return parser.parse_args()


def main():
    args = parse_args()

    items, parent_children, children = parse_tagged_items(args.input, args.tag)

    gen_ld(args.ld_output, items, args.alignment, parent_children, children)
    gen_cmake(args.cmake_output, items, args.alignment, parent_children, children)


if __name__ == "__main__":
    main()
