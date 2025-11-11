# Copyright 2025 NXP
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_NXP_MCXE31X_BOOT_HEADER)
  zephyr_library_sources(boot_header/boot_header.c)
  zephyr_library_include_directories(boot_header)
endif()
