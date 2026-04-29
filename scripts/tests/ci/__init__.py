# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

import os
import sys
from pathlib import Path

ZEPHYR_BASE = os.getenv("ZEPHYR_BASE", str(Path(__file__).parents[3]))

sys.path.insert(0, os.path.join(ZEPHYR_BASE, "scripts", "ci"))
