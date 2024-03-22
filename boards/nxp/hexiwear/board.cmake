# SPDX-License-Identifier: Apache-2.0

if (CONFIG_BOARD_HEXIWEAR_MK64F12)
  board_runner_args(pyocd "--target=k64f")
  board_runner_args(jlink "--device=MK64FN1M0xxx12")

  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
else()
  board_runner_args(jlink "--device=MKW40Z160xxx4")
  board_runner_args(pyocd "--target=kw40z4")

  include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
  include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
endif()
