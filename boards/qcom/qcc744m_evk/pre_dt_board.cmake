# Copyright (c) 2026 Canonical Limited
# SPDX-License-Identifier: Apache-2.0

# Suppress "unique_unit_address_if_enabled" to handle the following overlaps:
# - pin-controller@20000000 & gpio@20000000 & clock-controller@20000000
# - power-controller@2000f000 & regulator-soc@2000f000 & regulator-rt@2000f000 & regulator-aon@2000f000
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
