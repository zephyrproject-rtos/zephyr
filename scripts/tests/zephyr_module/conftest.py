# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

import sys
from pathlib import Path

# zephyr_module.py lives at the top of scripts/
sys.path.insert(0, str(Path(__file__).parents[2]))
