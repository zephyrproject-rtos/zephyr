#
# Copyright (c) 2021, NXP
#
# SPDX-License-Identifier: Apache-2.0
#

if(CONFIG_SOC_MIMXRT1176_CM7)
board_runner_args(pyocd "--target=mimxrt1170_cm7")
board_runner_args(jlink "--device=MIMXRT1176xxxA_M7" "--reset-after-load")
endif()



if(CONFIG_SOC_MIMXRT1176_CM4)
board_runner_args(pyocd "--target=mimxrt1170_cm4")
# Note: Please use evkmimxrt1170_connect_cm4_cm4side_sdram.jlinkscript if use SDRAM on board
board_runner_args(jlink "--device=MIMXRT1176xxxA_M4" "--tool-opt=-JLinkScriptFile ${ZEPHYR_BASE}/boards/arm/mimxrt1170_evk/evkmimxrt1170_connect_cm4_cm4side.jlinkscript" "--reset-after-load")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/pyocd.board.cmake)
