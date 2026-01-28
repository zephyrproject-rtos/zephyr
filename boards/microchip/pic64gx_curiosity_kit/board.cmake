# SPDX-License-Identifier: Apache-2.0

set(OPENOCD_USE_LOAD_IMAGE NO)

board_runner_args(openocd --use-elf --no-load)

if(CONFIG_BOARD_PIC64GX_CURIOSITY_KIT_PIC64GX1000_E51)
  board_runner_args(openocd --gdb-client-port=3333)
elseif(CONFIG_BOARD_PIC64GX_CURIOSITY_KIT_PIC64GX1000_U54)
  board_runner_args(openocd --gdb-client-port=3334)
endif()

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
