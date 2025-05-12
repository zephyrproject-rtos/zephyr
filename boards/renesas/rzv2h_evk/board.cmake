# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_RZV2H_EVK_R9A09G057H44GBG_CM33)
  board_runner_args(jlink "--device=R9A09G057H44_M33_0")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
