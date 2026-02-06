# Copyright 2026 NXP
# SPDX-License-Identifier: Apache-2.0

set(jlinkscript ${CMAKE_CURRENT_LIST_DIR}/jlinkscript)
if(CONFIG_BOARD_FRDM_IMXRT1186_MIMXRT1186_CM7)
  board_runner_args(jlink "--device=MIMXRT1186xxx8_M7" "--no-reset" "--tool-opt=-jlinkscriptfile ${jlinkscript}/frdmimxrt1186_cm7.jlinkscript")
elseif(CONFIG_BOARD_FRDM_IMXRT1186_MIMXRT1186_CM33)
  board_runner_args(jlink "--device=MIMXRT1186xxx8_M33" "--tool-opt=-jlinkscriptfile ${jlinkscript}/frdmimxrt1186_cm33.jlinkscript")
endif()

board_runner_args(linkserver "--device=MIMXRT1186xxxxx:FRDM-IMXRT1186")
if(CONFIG_BOARD_FRDM_IMXRT1186_MIMXRT1186_CM7)
  board_runner_args(linkserver "--core=cm7")
endif()

include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
