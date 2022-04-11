# Copyright (c) 2022, NXP
# SPDX-License-Identifier: Apache-2.0

# Suppress "spi_bus_bridge" on RT boards as the flexspi flash device has both an
# address and size cell.
list(APPEND EXTRA_DTC_FLAGS "-Wno-spi_bus_bridge")
