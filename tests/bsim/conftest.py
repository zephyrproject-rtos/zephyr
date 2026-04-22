# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys

zephyr_base = os.getenv("ZEPHYR_BASE")
if not zephyr_base:
    raise OSError("ZEPHYR_BASE environment variable is not set")

# Add the directory to PYTHONPATH
sys.path.insert(0, os.path.join(zephyr_base, "scripts", "pylib", "pytest-twister-harness", "src"))

pytest_plugins = [
    "twister_harness.plugin",
    "twister_harness_bsim.plugin",
]
