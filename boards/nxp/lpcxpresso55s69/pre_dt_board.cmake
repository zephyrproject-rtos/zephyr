# Copyright 2024 NXP
# SPDX-License-Identifier: Apache-2.0

# Suppress "unique_unit_address_if_enabled" to handle syscon
# clock address overlaps
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
