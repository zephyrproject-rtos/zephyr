"""
Make the shared TTCN-3 runner importable, and register the fixtures it holds.

Copyright (c) 2026 Nordic Semiconductor ASA

SPDX-License-Identifier: Apache-2.0

"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parents[2]))

from ttcn3_runner import network_lock  # noqa: E402, F401
