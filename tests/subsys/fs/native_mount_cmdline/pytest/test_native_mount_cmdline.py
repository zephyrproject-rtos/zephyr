# Copyright (c) 2026 Marcin Niestroj
#
# SPDX-License-Identifier: Apache-2.0

"""
Test native_mount command-line volume mounting.

Two testcase variants (default and banana_apple) exercise different host
directory names.  A single test function handles both by reading
CONFIG_NATIVE_EXTRA_CMDLINE_ARGS from the built .config file to discover
the actual host paths, rather than hardcoding them.
"""

import logging
import pathlib

from twister_harness import DeviceAdapter
from twister_harness.helpers.utils import find_in_config

logger = logging.getLogger(__name__)

CONTENT_A = "hello_from_dir_a"
CONTENT_B = "hello_from_dir_b"


def _parse_volume_mappings(build_dir: pathlib.Path) -> dict:
    """Return {zephyr_mount_point: host_path} by parsing the built .config.

    Reads CONFIG_NATIVE_EXTRA_CMDLINE_ARGS which looks like:
        CONFIG_NATIVE_EXTRA_CMDLINE_ARGS="-volume=./dir_a:/dir_a ..."

    Relative host paths are resolved against build_dir (the CWD used by
    twister when running native_sim binaries).
    """
    config_file = build_dir / "zephyr" / ".config"
    value = find_in_config(config_file, "CONFIG_NATIVE_EXTRA_CMDLINE_ARGS")
    mappings = {}
    for arg in value.split():
        if not arg.startswith("-volume="):
            continue
        # Split HOST:ZEPHYR on first colon only (POSIX host paths have no colon)
        host, zephyr = arg[len("-volume=") :].split(":", 1)
        host_path = pathlib.Path(host)
        if not host_path.is_absolute():
            host_path = (build_dir / host_path).resolve()
        mappings[zephyr] = host_path
    return mappings


def test_native_mount_cmdline(dut: DeviceAdapter):
    """Files written to two Zephyr mount points appear in the correct host dirs."""

    dut.readlines_until(regex="native_mount_cmdline: PASS", timeout=10.0)

    build_dir = pathlib.Path(dut.device_config.build_dir)
    mappings = _parse_volume_mappings(build_dir)

    logger.info("Volume mappings from .config: %s", mappings)
    assert mappings, "No -volume= entries found in CONFIG_NATIVE_EXTRA_CMDLINE_ARGS"

    for zephyr_path, expected_content in [("/dir_a", CONTENT_A), ("/dir_b", CONTENT_B)]:
        host_dir = mappings.get(zephyr_path)
        assert host_dir is not None, f"No volume mapping found for {zephyr_path}"

        host_file = host_dir / "hello.txt"
        assert host_file.exists(), f"File not found on host: {host_file}"

        actual = host_file.read_text()
        assert actual == expected_content, (
            f"Wrong content in {host_file}: expected {expected_content!r}, got {actual!r}"
        )
        logger.info("Verified %s (%s) -> %r", zephyr_path, host_file, actual)
