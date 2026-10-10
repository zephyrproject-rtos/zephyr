# Copyright (c) 2026 Canonical Limited
# SPDX-License-Identifier: Apache-2.0

# Suppress "unique_unit_address_if_enabled" to handle the following overlaps:
# - pin-controller@40000000 & clock-controller@40000000
# - power-controller@4000f000 & regulator-soc@4000f000 & regulator-rt@4000f000 & regulator-aon@4000f000
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
