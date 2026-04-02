# Copyright 2023 NXP
# SPDX-License-Identifier: Apache-2.0

# Suppress "spi_bus_bridge" as flexcomm node can be used as a SPI device.
list(APPEND EXTRA_DTC_FLAGS "-Wno-spi_bus_bridge")
