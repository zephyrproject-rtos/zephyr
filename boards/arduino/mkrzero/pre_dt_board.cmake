# Copyright (c) 2021 Linaro Limited
# SPDX-License-Identifier: Apache-2.0

# SPI is implemented via sercom so node name isn't spi@...
list(APPEND EXTRA_DTC_FLAGS "-Wno-spi_bus_bridge")

# Suppress "unique_unit_address_if_enabled" to handle the following overlaps:
# - /soc/pinmux@41004400 & /soc/gpio@41004400
# - /soc/pinmux@41004480 & /soc/gpio@41004480
list(APPEND EXTRA_DTC_FLAGS "-Wno-unique_unit_address_if_enabled")
