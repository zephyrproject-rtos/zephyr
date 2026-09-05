# Copyright (c) 2024 Nordic Semiconductor ASA
# Copyright (c) 2026 NUCODE Co., Ltd.
# SPDX-License-Identifier: Apache-2.0

# The nRF54L15 power, clock, XO, and LFCLK nodes intentionally overlap.
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
