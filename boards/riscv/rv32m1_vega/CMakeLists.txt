# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BT_CTLR_DEBUG_PINS)
  zephyr_library()
  zephyr_library_sources(board.c)
endif()
