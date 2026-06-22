# Copyright (c) 2021 Linaro Limited
# Copyright (c) 2026 Ezurio LLC
# SPDX-License-Identifier: Apache-2.0

# SPI is implemented via usart so node name isn't spi@...
list(APPEND EXTRA_DTC_FLAGS "-Wno-spi_bus_bridge")
