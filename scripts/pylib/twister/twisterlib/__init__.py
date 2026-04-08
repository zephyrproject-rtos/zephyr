# Copyright (c) 2025 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0
import os
import sys

ZEPHYR_BASE: str = os.environ.get("ZEPHYR_BASE", "")
if not ZEPHYR_BASE:
    sys.exit("$ZEPHYR_BASE environment variable undefined")
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/"))
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/build_helpers"))
# This is needed to load edt.pickle files.
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/dts/python-devicetree/src"))
