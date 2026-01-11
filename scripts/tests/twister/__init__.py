# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys
from pathlib import Path

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE", str(Path(__file__).parents[3]))

sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts/pylib/twister"))
sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts"))
