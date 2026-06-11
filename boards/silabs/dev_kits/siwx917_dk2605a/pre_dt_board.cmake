# Copyright (c) 2026 Silicon Laboratories Inc.
# SPDX-License-Identifier: Apache-2.0

# Suppress "unique_unit_address_if_enabled": pinctrl@46130000 and egpio@46130000
# map to the same hardware block and intentionally share the same address range.
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")

# Suppress "simple_bus_reg": siwx91x-soc-pd is a power domain node with no
# memory-mapped registers; it has no reg property by design.
list(APPEND EXTRA_DTC_FLAGS "-Wno-simple_bus_reg")
