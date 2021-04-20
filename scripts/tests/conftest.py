# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

import os
from pathlib import Path


os.environ["ZEPHYR_BASE"] = str(Path(__file__).parent / ".." / "..")
