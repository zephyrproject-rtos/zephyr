# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_LUCKFOX_LUME_T153_E907)
  zephyr_library()
  zephyr_library_sources(board.c)
endif()
