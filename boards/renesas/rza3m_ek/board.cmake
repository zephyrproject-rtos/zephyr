# Copyright (c) 2025 Renesas Electronics Corporation
# SPDX-License-Identifier: Apache-2.0

board_runner_args(jlink "--device=R9A07G066M04")
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

if(CONFIG_BUILD_WITH_TFA)
  set(TFA_PLAT "a3m_ek_nor")
endif()
