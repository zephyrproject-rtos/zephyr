# Copyright (c) 2022 Nordic Semiconductor
# SPDX-License-Identifier: Apache-2.0

# Suppress "unique_unit_address_if_enabled" to handle the following overlaps:
# - power@40000000 & clock@40000000 & nrf-mpu@40000000
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
