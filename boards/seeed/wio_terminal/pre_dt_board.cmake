# Copyright (c) 2023 Joel Guittet
# SPDX-License-Identifier: Apache-2.0

# SPI is implemented via sercom so node name isn't spi@...
list(APPEND EXTRA_DTC_FLAGS "-Wno-spi_bus_bridge")

# Suppress "unique_unit_address_if_enabled" to handle the following overlaps:
# - /soc/pinmux@41008000 & /soc/gpio@41008000
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
