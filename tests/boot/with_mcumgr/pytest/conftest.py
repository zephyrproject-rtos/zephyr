# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys

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
