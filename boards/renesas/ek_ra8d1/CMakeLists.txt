# Copyright (c) 2024 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

zephyr_include_directories(.)

zephyr_library()
zephyr_library_sources(board.c)

zephyr_linker_sources_ifdef(CONFIG_MEMC
  SECTIONS sdram.ld)
