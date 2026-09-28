# SPDX-FileCopyrightText: Copyright (c) 2026 Infineon Technologies AG,
# SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
#
# SPDX-License-Identifier: Apache-2.0

import os
import sys

zephyr_base = os.getenv("ZEPHYR_BASE")
if zephyr_base:
    sys.path.insert(
        0,
        os.path.join(zephyr_base, "scripts", "pylib", "pytest-twister-harness", "src"),
    )
else:
    raise OSError("ZEPHYR_BASE environment variable is not set")

pytest_plugins = ["twister_harness.plugin"]
