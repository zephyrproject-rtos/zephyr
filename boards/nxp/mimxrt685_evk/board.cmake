#
# Copyright 2020, 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_BOARD_MIMXRT685_EVK_MIMXRT685S_CM33)
  board_runner_args(jlink "--device=MIMXRT685S_M33" "--reset-after-load")
  board_runner_args(linkserver  "--device=MIMXRT685S:EVK-MIMXRT685")
elseif(CONFIG_BOARD_MIMXRT685_EVK_MIMXRT685S_HIFI4)
  board_runner_args(jlink "--device=MIMXRT685S_HiFi4" "--reset-after-load")
  board_runner_args(linkserver  "--device=MIMXRT685S:EVK-MIMXRT685")
endif()

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
