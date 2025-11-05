# Copyright (c) 2025 Core Devices LLC
# SPDX-License-Identifier: Apache-2.0

# keep first
set(sf32lb52_sftool_args "--chip=SF32LB52")

if(DEFINED BOARD_REVISION)
  if(BOARD_REVISION STREQUAL "n16r8")
    list(APPEND sf32lb52_sftool_args "--tool-opt=--memory nor")
  elseif(BOARD_REVISION STREQUAL "a128r8")
    list(APPEND sf32lb52_sftool_args "--tool-opt=--memory nand")
  endif()
endif()

board_runner_args(sftool ${sf32lb52_sftool_args})

# keep first
include(${ZEPHYR_BASE}/boards/common/sftool.board.cmake)
