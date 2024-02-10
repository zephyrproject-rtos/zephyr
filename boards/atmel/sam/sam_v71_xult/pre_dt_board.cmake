# Copyright (c) 2024 Gerson Fernando Budke <nandojve@gmail.com>
# SPDX-License-Identifier: Apache-2.0

# Suppress "unique_unit_address_if_enabled" to handle the following overlaps:
# - /soc/ethernet@40050000 & /soc/mdio@40050000
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
