# Copyright 2022-2023 NXP
# SPDX-License-Identifier: Apache-2.0

board_runner_args(trace32
  "--startup-args"
  "elfFile=${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}"
  "rtu=${CONFIG_NXP_S32_RTU_INDEX}"
)

board_runner_args(nxp_s32dbg
  "--soc-family-name" "s32z2e2"
  "--soc-name" "S32Z270"
)

if(CONFIG_DCLS)
  board_runner_args(trace32 "lockstep=yes")
  board_runner_args(nxp_s32dbg "--core-name" "R52_${CONFIG_NXP_S32_RTU_INDEX}_0_LS")
else()
  board_runner_args(trace32 "lockstep=no")
  board_runner_args(nxp_s32dbg "--core-name" "R52_${CONFIG_NXP_S32_RTU_INDEX}_0")
endif()

include(${ZEPHYR_BASE}/boards/common/nxp_s32dbg.board.cmake)
include(${ZEPHYR_BASE}/boards/common/trace32.board.cmake)
