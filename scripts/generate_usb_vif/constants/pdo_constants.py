#!/usr/bin/env python3

# Copyright (c) 2022 The Chromium OS Authors
# SPDX-License-Identifier: Apache-2.0

"""This file contains PDO constants defined to be used by generate_vif.py"""

# PDO
PDO_TYPE_FIXED = 0
PDO_TYPE_BATTERY = 1
PDO_TYPE_VARIABLE = 2
PDO_TYPE_AUGUMENTED = 3

PDO_TYPES = {
    PDO_TYPE_FIXED: "Fixed",
    PDO_TYPE_BATTERY: "Battery",
    PDO_TYPE_VARIABLE: "Variable",
    PDO_TYPE_AUGUMENTED: "Augmented",
}
