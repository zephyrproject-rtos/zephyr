# SPDX-License-Identifier: Apache-2.0

if(CONFIG_BOARD_SHAKTI_VAJRA)
  board_runner_args(openocd "--use-elf" "--config=${ZEPHYR_BASE}/boards/riscv/shakti-vajra/support/shakti_vajra_100t.cfg")

  include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)

endif()
