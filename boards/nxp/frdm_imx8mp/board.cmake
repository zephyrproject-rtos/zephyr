# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright (c) 2026 Filip Stojanovic <filipembedded@gmail.com>

if(CONFIG_SOC_MIMX8ML8_M7)
  board_runner_args(jlink "--device=MIMX8ML8_M7")
  include("${ZEPHYR_BASE}/boards/common/jlink.board.cmake")
endif()
