# Copyright (C) 2026 Cody Gu <gujiaqi@iscas.ac.cn>
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_XIP)
  board_runner_args(openocd "--file-type=hex")
else()
  board_runner_args(openocd "--file-type=elf" "--cmd-reset-halt=halt")
endif()

board_runner_args(openocd
  "--openocd-search=${ZEPHYR_HAL_HPMICRO_MODULE_DIR}/hpm_sdk/boards/openocd"
  "--cmd-pre-init=adapter speed 500"
  "--target-handle=_TARGET0"
)

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
