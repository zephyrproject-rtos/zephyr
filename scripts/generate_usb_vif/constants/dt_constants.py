#!/usr/bin/env python3

# Copyright (c) 2022 The Chromium OS Authors
# SPDX-License-Identifier: Apache-2.0

"""This file contains device tree constants defined to be used by generate_vif.py"""

SINK_PDOS = "sink-pdos"
PD_DISABLE = "pd-disable"
POWER_ROLE = "power-role"

DT_VIF_ELEMENTS = {
    SINK_PDOS: "SnkPdoList",
}
