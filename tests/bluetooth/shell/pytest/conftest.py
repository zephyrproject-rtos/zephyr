# Copyright (c) 2023 Nordic Semiconductor ASA
# Copyright (c) 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys

# Add the directory to PYTHONPATH
zephyr_base = os.path.abspath(os.getenv("ZEPHYR_BASE"))
if zephyr_base:
    sys.path.insert(
        0, os.path.join(zephyr_base, "scripts")
    )  # import zephyr_module in environment.py
    sys.path.insert(0, os.path.join(zephyr_base, "scripts/pylib"))
    sys.path.insert(0, os.path.join(zephyr_base, "scripts", "pylib", "twister"))
else:
    raise OSError("ZEPHYR_BASE environment variable is not set")

pytest_plugins = [
    "poc_plugins.multi_device_fixtures",
]
