#
# Copyright (c) 2019, NXP
#
# SPDX-License-Identifier: Apache-2.0
#
set_ifndef(OPENSDA_FW jlink)

if(OPENSDA_FW STREQUAL jlink)
  set_ifndef(BOARD_DEBUG_RUNNER jlink)
  set_ifndef(BOARD_FLASH_RUNNER jlink)
endif()

board_runner_args(jlink "--device=MIMXRT1015")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
