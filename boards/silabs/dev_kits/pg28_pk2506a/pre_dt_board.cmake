# Copyright (c) 2025 Christoph Jans
# SPDX-License-Identifier: Apache-2.0

# SPI is implemented via usart so node name isn't spi@...
list(APPEND EXTRA_DTC_FLAGS "-Wno-spi_bus_bridge")
