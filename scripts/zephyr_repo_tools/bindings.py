#!/usr/bin/env python3
#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import logging
import re
from pathlib import Path
from typing import Any

from zephyr_repo_tools.iterfiles import iter_files, subpath_after_marker

logger = logging.getLogger(__name__)


class ZephyrDTSBindings:
    """
    Represents DTS bindings and provides utilities for resolving types and compatibilities.
    """

    def __init__(self, bindings_path: Path) -> None:
        """
        Initialize the bindings index by scanning YAML files in the given path.

        Args:
            bindings_path (Path): Path to the directory containing DTS binding files.
        """
        logger.info(f"ZephyrDTSBindings: Processing bindings: {bindings_path}")

        self.bindings_idx: dict[str, dict[str, Any]] = {}
        for f in iter_files(bindings_path, [".yaml"]):
            # TODO: Open the YAML and read the title/description to store
            binding = {
                "type": subpath_after_marker(f, "bindings"),
                "dt_drv_compat": self._binding_to_dt_drv_compat(f.stem),
                "compatible": f.stem,
                "path": f,
            }
            # Index by both dt_drv_compat and compatible name
            self.bindings_idx[binding["dt_drv_compat"]] = binding
            self.bindings_idx[binding["compatible"]] = binding

    def __str__(self) -> str:
        """Return a string representation of the bindings object."""
        return "ZephyrDTSBindings()"

    def _binding_to_dt_drv_compat(self, name: str) -> str:
        """
        Convert a binding name into a valid dt_drv_compat string.

        Args:
            name (str): The binding name.

        Returns:
            str: A sanitized compatibility string.
        """
        out = re.sub(r"[^0-9A-Za-z]", "_", name)
        if out and out[0].isdigit():
            out = "_" + out
        return out

    def get_type(self, binding: str) -> str | None:
        """
        Get the type associated with a given binding.

        Args:
            binding (str): The binding name or dt_drv_compat key.

        Returns:
            str | None: The type if found, otherwise None.
        """
        bentry = self.bindings_idx.get(binding)
        return bentry["type"] if bentry else None

    def compat_str(self, binding: str | Path) -> str:
        """
        Convert a binding name or Path to its dt_drv_compat string.

        Args:
            binding (str | Path): The binding name or file path.

        Returns:
            str: The dt_drv_compat string.
        """
        try:
            return self._binding_to_dt_drv_compat(binding.stem)  # Path case
        except AttributeError:
            return self._binding_to_dt_drv_compat(binding)  # str case
