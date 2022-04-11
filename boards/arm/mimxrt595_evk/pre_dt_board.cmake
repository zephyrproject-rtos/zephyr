# Copyright (c) 2022, NXP
# SPDX-License-Identifier: Apache-2.0

# Suppress "simple_bus_reg" on RT6XX boards as all GPIO ports use the same register.
list(APPEND EXTRA_DTC_FLAGS "-Wno-simple_bus_reg")
# The flexspi uses a different size-cells value than most spi controllers, since
# the base address for the flexspi memory region is stored there. Suppress this
# warning.
list(APPEND EXTRA_DTC_FLAGS "-Wno-spi_bus_bridge")
