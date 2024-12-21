#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: BSD-3-Clause
#

# Check if ZEPHYR_BASE is set
if(NOT DEFINED ENV{ZEPHYR_BASE})
    message(FATAL_ERROR "ZEPHYR_BASE environment variable is not set. Please set it to the Zephyr base directory.")
endif()

if (CONFIG_NRF70_BUSLIB)
  zephyr_library_named(nrf70-buslib)
  zephyr_library_include_directories(
    inc
  )
  zephyr_library_sources(
    rpu_hw_if.c
    device.c
  )
  zephyr_library_sources_ifdef(CONFIG_NRF70_ON_QSPI
    qspi_if.c
  )
  zephyr_library_sources_ifdef(CONFIG_NRF70_ON_SPI
    spi_if.c
  )
endif()
