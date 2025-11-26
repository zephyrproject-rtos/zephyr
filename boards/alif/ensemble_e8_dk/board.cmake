# Copyright (c) 2025 Alif Semiconductor
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_AE822FA0E5597LS0_RTSS_HP)
  board_runner_args(jlink "--device=AE822FA0E5597LS0_M55_HP" "--speed=4000")
endif()

if(CONFIG_SOC_AE822FA0E5597LS0_RTSS_HE)
  board_runner_args(jlink "--device=AE822FA0E5597LS0_M55_HE" "--speed=4000")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
