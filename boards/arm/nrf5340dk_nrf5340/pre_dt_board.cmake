# Copyright (c) 2021 Nordic Semiconductor ASA
# SPDX-License-Identifier: Apache-2.0

# This SoC has duplicate unit addresses for some peripherals, such as
# KMU and NVMC.
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
