#
# Copyright 2024-2025 NXP
#
# SPDX-License-Identifier: Apache-2.0
#

set(RT1180_BOARD_DIR
"${ZEPHYR_HAL_NXP_MODULE_DIR}/mcux/mcux-sdk/boards/evkmimxrt1180")
# Note1: Suggest developers use Secure Provisioning Tool(SPT) to download RT1180 image
# SPT can be downloaded on NXP web: https://www.nxp.com/design/design-center/software/development-software/mcuxpresso-software-and-tools-/mcuxpresso-secure-provisioning-tool:MCUXPRESSO-SECURE-PROVISIONING
# Details about the usage of SPT on MIMXRT1180-EVK board can be referred on chapter 7 of getting start with Mcuxpresso SDK for MIMXRT1180-EVK doc in SDK package.
if(CONFIG_SOC_MIMXRT1189_CM33 OR CONFIG_SECOND_CORE_MCUX)
board_runner_args(linkserver  "--device=MIMXRT1189xxxxx:MIMXRT1180-EVK")
board_runner_args(jlink "--device=MIMXRT1189xxx8_M33" "--reset-after-load" "--tool-opt=-jlinkscriptfile ${RT1180_BOARD_DIR}/jlinkscript/evkmimxrt1180_cm33.jlinkscript")
elseif(CONFIG_SOC_MIMXRT1189_CM7)
# Note: Only support run cm7 image when debugging due to default boot core on board is cm33 core
board_runner_args(linkserver  "--device=MIMXRT1189xxxxx:MIMXRT1180-EVK")
board_runner_args(linkserver "--core=cm7")
board_runner_args(jlink "--device=MIMXRT1189xxx8_M7" "--speed=4000" "--no-reset" "--tool-opt=-jlinkscriptfile ${RT1180_BOARD_DIR}/jlinkscript/evkmimxrt1180_cm7.jlinkscript" "--tool-opt=-ir")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
include(${ZEPHYR_BASE}/boards/common/linkserver.board.cmake)
