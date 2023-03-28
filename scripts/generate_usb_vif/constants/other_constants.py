#!/usr/bin/env python3

# Copyright (c) 2022 The Chromium OS Authors
# SPDX-License-Identifier: Apache-2.0

"""This file contains generic constants defined to be used by generate_vif.py"""

NAME = "name"
VALUE = "value"
TEXT = "text"
ATTRIBUTES = "attributes"
CHILD = "child"
TRUE = "true"
FALSE = "false"

PD_PORT_TYPE_VALUES = {
    "sink": ("0", "Consumer Only"),
    "source": ("3", "Provider Only"),
    "dual": ("4", "DRP"),
}

TYPE_C_STATE_MACHINE_VALUES = {
    "sink": ("1", "SNK"),
    "source": ("0", "SRC"),
    "dual": ("2", "DRP"),
}

FR_SWAP_REQD_TYPE_C_CURRENT_AS_INITIAL_SOURCE_VALUES = {
    0: "FR_Swap not supported",
    1: "Default USB Power",
    2: "1.5A @ 5V",
    3: "3A @ 5V",
}
