# Copyright (c) 2025 Silicon Signals Pvt. Ltd.
# SPDX-License-Identifier: Apache-2.0

# SPI functionality is provided via a USART peripheral; the node is named
# usart@..., not spi@... To suppress the 'spi_bus_reg: Failed prerequisite
# "spi_bus_bridge"' warning, the -Wno-spi_bus_bridge flag is applied.
list(APPEND EXTRA_DTC_FLAGS "-Wno-spi_bus_bridge")
