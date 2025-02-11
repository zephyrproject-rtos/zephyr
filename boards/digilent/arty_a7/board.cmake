# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_ARTY_A7_DESIGNSTART_FPGA_CORTEX_M1)
  board_runner_args(openocd "--use-elf" "--config=${BOARD_DIR}/support/openocd_arty_a7_arm_designstart_m1.cfg")
  board_runner_args(jlink "--device=Cortex-M1" "--reset-after-load")
elseif(CONFIG_BOARD_ARTY_A7_DESIGNSTART_FPGA_CORTEX_M3)
  board_runner_args(openocd "--use-elf" "--config=${BOARD_DIR}/support/openocd_arty_a7_arm_designstart_m3.cfg")
  board_runner_args(jlink "--device=Cortex-M3" "--reset-after-load")
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
