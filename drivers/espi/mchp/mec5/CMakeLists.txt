# Copyright Microchip Technology Inc.
# SPDX-License-Identifier: Apache-2.0

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_ESPI_FLASH_CHANNEL flash_chan.c)
zephyr_library_sources_ifdef(CONFIG_ESPI_OOB_CHANNEL oob_chan.c)
zephyr_library_include_directories(
  ${ZEPHYR_BASE}/drivers/espi
)
