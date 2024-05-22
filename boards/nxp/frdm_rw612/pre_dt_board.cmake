# Copyright 2023 NXP
# SPDX-License-Identifier: Apache-2.0

# Suppress "simple_bus_reg" on RW6XX boards as all GPIO ports use the same register.
list(APPEND EXTRA_DTC_FLAGS "-Wno-simple_bus_reg")
# Suppress "spi_bus_bridge" as flexcomm node can be used as a SPI device.
list(APPEND EXTRA_DTC_FLAGS "-Wno-spi_bus_bridge")
