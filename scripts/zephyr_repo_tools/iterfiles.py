#!/usr/bin/env python3
#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

from collections.abc import Iterable
from pathlib import Path


def iter_files(root: Path, exts: set[str]) -> Iterable[Path]:
    """
    Yield files under the given root path that match the specified extensions.

    Args:
        root (Path): The root path to search. Can be a file or directory.
        exts (set[str]): Set of file extensions to match (e.g., {".yaml", ".dts"}).

    Yields:
        Path: Paths to files matching the given extensions.
    """
    if root.is_file():
        # Yield the file regardless of extension (or uncomment filter if needed)
        yield root
        return

    if root.is_dir():
        # Recursively search for files matching extensions
        for p in root.rglob("*"):
            if p.is_file() and p.suffix.lower() in exts:
                yield p
        return

    # If the path doesn't exist or is special, do nothing


def subpath_after_marker(p: Path, marker: str) -> str:
    """
    Return the subpath after a given marker in the path, excluding the filename.

    Args:
        p (Path): The full path.
        marker (str): The marker directory name to search for.

    Returns:
        str: The subpath after the marker, up to (but not including) the filename.

    Raises:
        ValueError: If the marker is not found in the path.
    """
    parts = p.parts
    try:
        i = parts.index(marker)
    except ValueError as err:
        raise ValueError(f"'{marker}' not found in path: {p}") from err

    # Everything after 'marker' up to (but not including) the filename
    return Path(*parts[i + 1 : -1]).as_posix()
