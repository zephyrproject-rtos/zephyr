#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

"""Print a random default password for the SSH sample, invoked from Kconfig."""

import base64
import os
import sys

# No trailing newline: the output is used verbatim as the Kconfig string value.
sys.stdout.write(base64.b64encode(os.urandom(12)).decode()[:15])
