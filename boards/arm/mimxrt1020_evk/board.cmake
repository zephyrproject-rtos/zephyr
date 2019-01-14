#
# Copyright (c) 2018, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

set_ifndef(OPENSDA_FW jlink)

if(OPENSDA_FW STREQUAL jlink)
  set_ifndef(BOARD_DEBUG_RUNNER jlink)
  set_ifndef(BOARD_FLASH_RUNNER jlink)
elseif(OPENSDA_FW STREQUAL daplink)
  set_ifndef(BOARD_DEBUG_RUNNER pyocd)
  set_ifndef(BOARD_FLASH_RUNNER pyocd)
endif()

board_runner_args(pyocd "--target=mimxrt1020")
board_runner_args(jlink "--device=MIMXRT1021xxx5A")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
