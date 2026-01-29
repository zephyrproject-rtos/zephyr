#!/usr/bin/env python3
#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import logging
import os
import re
from collections import Counter
from pathlib import Path

from zephyr_repo_tools.bindings import ZephyrDTSBindings

logger = logging.getLogger(__name__)


def resolve_include_path(
    filename: str,
    current_dir: Path | str | None,
    include_paths: list[Path],
) -> Path | None:
    """
    Resolve the full path of an included file based on search rules.

    Args:
        filename (str): The name of the file to include.
        current_dir (Optional[Path]): Current directory for quoted includes.
        include_paths (list[Path]): list of include paths for searching.

    Returns:
        Optional[Path]: The resolved file path if found, otherwise None.
    """
    # For quoted includes, check current directory first
    if current_dir:
        candidate = Path(current_dir) / filename
        if candidate.is_file():
            return candidate

    # Search in provided include paths
    for path in include_paths:
        candidate = Path(path) / filename  # Ensure path is a Path object
        if candidate.is_file():
            return candidate

    return None


def extract_compatible_strings_from_dts(
    file_path: Path,
    include_paths: list[Path],
    visited_files: set[str],
    bindings: "ZephyrDTSBindings",
) -> Counter:
    """
    Extract compatible strings from a DTS file and its included files.

    Args:
        file_path (Path): Path to the DTS file.
        include_paths (list[Path]): Include paths for resolving #include directives.
        visited_files (set[str]): Tracks already processed files to avoid recursion.
        bindings (ZephyrDTSBindings): Bindings object for validating compatible strings.

    Returns:
        Counter: A Counter mapping compatible strings to their occurrence counts.
    """
    logger.info(f"  process {file_path}")
    compatible_strings = Counter()

    abs_path = os.path.abspath(file_path)
    if abs_path in visited_files:
        return compatible_strings  # Already parsed

    visited_files.add(abs_path)

    try:
        with open(file_path, encoding="utf-8") as f:
            lines = f.readlines()
    except FileNotFoundError:
        logging.debug(f"Warning: File not found: {file_path}")
        return compatible_strings

    current_dir = Path(file_path)

    for line in lines:
        # Remove block and line comments
        line = re.sub(r"/\*.*?\*/", "", line)
        line = re.sub(r"//.*", "", line)

        # Match all compatible strings in the line
        match = re.search(
            r'compatible\s*=\s*((?:"[^"]*"(?:\s*,\s*)?)+)\s*;',
            line,
            flags=re.DOTALL,
        )
        if match:
            values = re.findall(r'"([^"]*)"', match.group(1))
            for v in values:
                # Some compatibles include multiple entries, e.g.:
                #
                #     compatible = "nxp,imx-lpuart", "nxp,lpuart"
                #
                # The first may be a platform-level compat not yet used.
                # To avoid storing invalid bindings, only keep entries
                # that resolve to a known type via bindings.get_type().
                #
                logger.debug({v})
                if bindings.get_type(v):
                    compatible_strings.update([v])
                    # Keep just the first valid one. It skews
                    # the numbers when there are multiple.
                    break

        # Handle includes
        include_match = re.search(r'#include\s*[<"]\s*(.+?)\s*[">]', line)
        if include_match:
            include_file = include_match.group(1)

            # Determine search order based on include style
            if line.strip().startswith('#include "'):
                resolved_path = resolve_include_path(include_file, current_dir, include_paths)
            else:
                resolved_path = resolve_include_path(include_file, None, include_paths)

            if resolved_path:
                compatible_strings.update(
                    extract_compatible_strings_from_dts(
                        resolved_path,
                        include_paths,
                        visited_files,
                        bindings,
                    )
                )
            else:
                logging.debug(f"Could not resolve include: {include_file}")

    return compatible_strings


class ZephyrDTSCompats:
    """
    Represents a Zephyr DTS file and its compatible strings.

    The DTS file will be processed for compatible strings. The parser
    will walk and process the included *.dtsi files using the include_paths
    to find the file.

    Attributes:
        dts_file (Path): The DTS file path.
        include_paths (list[Path]): Include paths for DTS parsing.
        compatible_strings (dict[str, int]): Mapping of compatible strings to counts.
    """

    def __init__(
        self,
        dts_file: Path,
        include_paths: list[str],
        bindings: "ZephyrDTSBindings",
    ) -> None:
        """
        Initialize a ZephyrDTSCompats instance.

        Args:
            dts_file (Path): Path to the DTS file.
            include_paths (list[Path]): Include paths for DTS parsing.
            bindings (ZephyrDTSBindings): Bindings object for resolving types.
        """
        logger.info(f"ZephyrDTSCompats({dts_file})")

        self.dts_file = dts_file

        self.include_paths = [Path(p) for p in include_paths]

        visited_files: set[Path] = set()
        self.compatible_strings: dict[str, int] = extract_compatible_strings_from_dts(
            self.dts_file,
            self.include_paths,
            visited_files,
            bindings,
        )

    def __str__(self) -> str:
        """Return a human-readable representation of the object."""
        return f"ZephyrDTSCompats(file: {self.dts_file}, compats: {self.compatible_strings})"

    def __repr__(self) -> str:
        """Return the string representation for debugging."""
        return self.__str__()

    def __lt__(self, other: "ZephyrDTSCompats") -> bool:
        """Compare boards by filename for sorting."""
        return self.dts_file.name < other.dts_file.name

    def __eq__(self, other: object) -> bool:
        """Check equality based on filename."""
        if not isinstance(other, ZephyrDTSCompats):
            return NotImplemented
        return self.dts_file.name == other.dts_file.name

    def __hash__(self) -> int:
        """Return a hash based on filename."""
        return hash(self.dts_file.name)

    @property
    def filename(self) -> str:
        """Return the DTS file name."""
        return self.dts_file.name

    @property
    def filestem(self) -> str:
        """Return the DTS file stem (name without extension)."""
        return self.dts_file.stem

    @property
    def compatibles(self) -> dict[str, int]:
        """Return the mapping of compatible strings to counts."""
        return self.compatible_strings

    def get_compat(self, compat: str) -> str:
        """
        Get the count for a specific compatible string.

        Args:
            compat (str): The compatible string.

        Returns:
            str: The count as a string, or an empty string if not found.
        """
        return self.compatible_strings.get(compat, "")
