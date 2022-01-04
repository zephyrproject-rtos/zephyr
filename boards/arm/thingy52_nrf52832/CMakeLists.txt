# SPDX-License-Identifier: Apache-2.0

if(CONFIG_GPIO)
zephyr_library()
zephyr_library_sources(board.c)
endif()
