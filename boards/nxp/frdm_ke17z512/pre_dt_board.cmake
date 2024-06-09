# Copyright (c) 2024, NXP
# SPDX-License-Identifier: Apache-2.0

# Suppress "simple_bus_reg" on KE1xz boards as all GPIO share an interrupt.
list(APPEND EXTRA_DTC_FLAGS "-Wno-simple_bus_reg")
