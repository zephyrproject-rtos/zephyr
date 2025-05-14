#
# Copyright 2025 NXP
#
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_SOC_MIMX9352_A55)

board_runner_args(jlink "--device=MIMX9352_A55_0" "--no-reset" "--flash-script=${CMAKE_BINARY_DIR}/zephyr/runner.jlinkscript")

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)

endif()
