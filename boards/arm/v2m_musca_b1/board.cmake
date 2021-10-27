#SPDX-License-Identifier: Apache-2.0

board_set_debugger_ifnset(pyocd)
board_set_flasher_ifnset(pyocd)

board_runner_args(pyocd "--target=musca_b1")

if(CONFIG_BOARD_SUPPORT_REMOTE_ENDPOINT AND CONFIG_BOARD_MUSCA_B1)
  set(BOARD_REMOTE "v2m_musca_b1_ns")
endif()

include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
