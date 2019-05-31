# SPDX-License-Identifier: Apache-2.0

set_ifndef(OPENSDA_FW jlink)

if(OPENSDA_FW STREQUAL jlink)
  board_set_debugger_ifnset(jlink)
  board_set_flasher_ifnset(jlink)
elseif(OPENSDA_FW STREQUAL daplink)
  board_set_debugger_ifnset(pyocd)
  board_set_flasher_ifnset(pyocd)
endif()

board_runner_args(pyocd "--target=k64f")
board_runner_args(jlink "--device=MK64FN1M0xxx12")

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
