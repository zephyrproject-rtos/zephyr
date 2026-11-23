# SPDX-FileCopyrightText: Copyright Alif Semiconductor
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_AE822FA0E5597BS0_RTSS_HP)
  board_runner_args(jlink "--device=AE822FA0E5597BS0_M55_HP" "--speed=4000")
  board_runner_args(alif_flash "--device=AE822FA0E5597BS0_HP")
endif()

if(CONFIG_SOC_AE822FA0E5597BS0_RTSS_HE)
  board_runner_args(jlink "--device=AE822FA0E5597BS0_M55_HE" "--speed=4000")
  board_runner_args(alif_flash "--device=AE822FA0E5597BS0_HE")
endif()

if(CONFIG_SOC_AE822FA0E5597BS0_APSS)
  board_runner_args(jlink "--device=AE822FA0E5597BS0_A32_0" "--speed=4000")
  board_runner_args(alif_flash "--device=AE822FA0E5597BS0_A320")
endif()

include(${ZEPHYR_BASE}/boards/common/alif_flash.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
