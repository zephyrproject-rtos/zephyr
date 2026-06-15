#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_BOARD_FRDM_MCXW72)
  zephyr_library()
  zephyr_library_sources(board.c)
endif()
