#
# Copyright 2026 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_MIMXRT798S_CM33_CPU0 OR CONFIG_SECOND_CORE_MCUX)
  board_runner_args(jlink "--device=MIMXRT798S_M33_0" "--reset-after-load")
elseif(CONFIG_SOC_MIMXRT798S_CM33_CPU1)
  board_runner_args(jlink "--device=MIMXRT798S_M33_1")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
