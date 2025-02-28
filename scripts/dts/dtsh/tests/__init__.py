# Copyright (c) 2024 Christophe Dufaza <chris@openmarl.org>
# SPDX-License-Identifier: Apache-2.0

"""Unit tests package."""

import os
import sys
from pathlib import Path

# Somewhat ugly hack allowing unit tests to import the
# python-devicetree library.
THIS_ZEPHYR = Path(__file__).parent.parent.parent.parent.parent
ZEPHYR_BASE = Path(os.environ.get('ZEPHYR_BASE', THIS_ZEPHYR))
PYTHON_DEVICETREE = ZEPHYR_BASE / "scripts" / "dts" / "python-devicetree" / "src"

sys.path.append(os.fspath(PYTHON_DEVICETREE))
