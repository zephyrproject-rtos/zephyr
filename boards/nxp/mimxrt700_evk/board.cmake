#
# Copyright 2024 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_MIMXRT798S_HIFI4)
  board_runner_args(jlink "--device=MIMXRT798S_hifi4" "--reset-after-load")
  board_runner_args(linkserver  "--device=MIMXRT798S:EVK-MIMXRT798")
elseif(CONFIG_SOC_MIMXRT798S_HIFI1)
  board_runner_args(jlink "--device=MIMXRT798S_hifi1" "--reset-after-load")
  board_runner_args(linkserver  "--device=MIMXRT798S:EVK-MIMXRT798")
endif()

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
