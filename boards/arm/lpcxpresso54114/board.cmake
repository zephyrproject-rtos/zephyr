#
# Copyright (c) 2017, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_BOARD_LPCXPRESSO54114_M4)
board_runner_args(jlink "--device=LPC54114J256_M4" "--reset-after-load")
elseif(CONFIG_BOARD_LPCXPRESSO54114_M0)
board_runner_args(jlink "--device=LPC54114J256_M0" "--reset-after-load")
endif()

if(CONFIG_BOARD_SUPPORT_REMOTE_ENDPOINT AND CONFIG_BOARD_LPCXPRESSO54114_M4)
  set(BOARD_REMOTE "lpcxpresso54114_m0")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
