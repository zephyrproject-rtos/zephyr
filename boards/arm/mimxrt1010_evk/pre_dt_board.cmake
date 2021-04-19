# Copyright 2021 NXP
# SPDX-License-Identifier: Apache-2.0

# Allow non-zero #size-cells so we can define the size of memory-mapped FlexSPI
# devices
list(APPEND EXTRA_DTC_FLAGS "-Wno-spi_bus_bridge")
