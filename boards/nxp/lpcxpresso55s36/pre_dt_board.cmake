# Copyright 2022 NXP
# SPDX-License-Identifier: Apache-2.0

# Suppress "simple_bus_reg" on LPC boards as all GPIO ports use the same register.
list(APPEND EXTRA_DTC_FLAGS "-Wno-simple_bus_reg")

# Suppress "unique_unit_address_if_enabled" to handle the following overlaps:
# - /soc/peripheral@40000000/syscon@0 & /soc/peripheral@40000000/gpio@0
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
