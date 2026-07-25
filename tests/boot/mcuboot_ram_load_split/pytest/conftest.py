# Copyright (c) 2026 The Zephyr Project Contributors
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys

zephyr_base = os.environ["ZEPHYR_BASE"]
sys.path.insert(0, os.path.join(zephyr_base, "scripts", "pylib", "pytest-twister-harness", "src"))

pytest_plugins = ["twister_harness.plugin"]
