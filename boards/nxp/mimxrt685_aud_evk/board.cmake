# SPDX-FileCopyrightText: Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_MIMXRT685_AUD_EVK_MIMXRT685S_CM33)
  board_runner_args(jlink "--device=MIMXRT685S_M33" "--reset-after-load")
  board_runner_args(linkserver  "--device=MIMXRT685S:MIMXRT685-AUD-EVK")
endif()

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
