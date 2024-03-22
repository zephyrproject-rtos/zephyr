#
# Copyright (c) 2021, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_MIMXRT1166_CM7 OR CONFIG_SECOND_CORE_MCUX)
board_runner_args(pyocd "--target=mimxrt1160_cm7")
board_runner_args(jlink "--device=MIMXRT1166xxx6_M7" "--reset-after-load")
elseif(CONFIG_SOC_MIMXRT1166_CM4)
board_runner_args(pyocd "--target=mimxrt1160_cm4")
# Note: Please use JLINK above V7.50 (Only support run cm4 image when debugging due to default boot core on board is cm7 core)
board_runner_args(jlink "--device=MIMXRT1166xxx6_M4")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
