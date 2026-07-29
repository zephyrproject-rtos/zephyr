# Copyright (c) 2026 Intercreate, Inc.
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys

import pytest

# Add the directory to PYTHONPATH
zephyr_base = os.getenv("ZEPHYR_BASE")
if zephyr_base:
    sys.path.insert(
        0, os.path.join(zephyr_base, "scripts", "pylib", "pytest-twister-harness", "src")
    )
else:
    raise OSError("ZEPHYR_BASE environment variable is not set")

pytest_plugins = [
    "twister_harness.plugin",
]


def pytest_addoption(parser: pytest.Parser) -> None:
    parser.addoption(
        "--expected-key-id",
        type=int,
        required=True,
        help="key_id MCUboot is expected to validate the application against",
    )
    parser.addoption(
        "--resign-key",
        default="",
        help=(
            "filename of a key (resolved against the build's signing-key "
            "directory) to re-sign the application with before loading it, "
            "modelling a production custodian signing the release binary"
        ),
    )
