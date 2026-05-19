# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_LATE_INIT_HOOK)
zephyr_library()
zephyr_library_sources(board.c)
endif()
