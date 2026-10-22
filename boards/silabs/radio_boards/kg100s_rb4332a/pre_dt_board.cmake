# Copyright (c) 2026 Martin Moya <martin.moya@focus.uy>
# SPDX-License-Identifier: Apache-2.0

# SPI is implemented via usart/eusart so node name isn't spi@...
list(APPEND EXTRA_DTC_FLAGS "-Wno-spi_bus_bridge")
