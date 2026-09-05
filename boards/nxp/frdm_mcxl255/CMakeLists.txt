# Copyright 2025-2026 NXP
# SPDX-License-Identifier: Apache-2.0

zephyr_library()
zephyr_library_sources(board.c)

if(CONFIG_MCUBOOT_BOOTUTIL_LIB)
  zephyr_library_sources(mcuboot_hooks/mcuboot_hooks.c)
  zephyr_library_include_directories(mcuboot_hooks)
endif()
