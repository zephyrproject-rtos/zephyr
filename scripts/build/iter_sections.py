#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Shared iterable-section parsing helpers."""

import json


def parse_tagged_items(filepath, tag):
    """Parse section items and inheritance relationships from a tagged JSON list."""
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

    for parent in parent_children:
        parent_children[parent].sort()

    return items, parent_children, children
