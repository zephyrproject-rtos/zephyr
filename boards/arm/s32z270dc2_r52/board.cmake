# Copyright 2022 NXP
# SPDX-License-Identifier: Apache-2.0

board_set_flasher_ifnset(trace32)
board_set_debugger_ifnset(trace32)

board_runner_args(trace32 "--startup-args" "elfFile=${PROJECT_BINARY_DIR}/${KERNEL_ELF_NAME}")

if(CONFIG_ISA_THUMB2)
board_runner_args(trace32 "thumb=yes")
else()
board_runner_args(trace32 "thumb=no")
endif()

if(CONFIG_BOARD_S32Z270DC2_RTU0_R52)
board_runner_args(trace32 "rtu=0")
elseif(CONFIG_BOARD_S32Z270DC2_RTU1_R52)
board_runner_args(trace32 "rtu=1")
endif()

include(${ZEPHYR_BASE}/boards/common/trace32.board.cmake)
