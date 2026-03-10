# Copyright (c) 2025 Infineon Technologies AG,
# or an affiliate of Infineon Technologies AG.
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_INFINEON_CAT1_QSPI_FLASH OR CONFIG_PM)
  zephyr_include_directories(config)
  zephyr_library_sources(config/qspi_memslot.c)
endif()
