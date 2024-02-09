# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS renode qemu)
set(RENODE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/support/hifive1.resc)
set(RENODE_UART sysbus.uart0)

set(QEMU_binary_suffix riscv32)
set(QEMU_CPU_TYPE_${ARCH} riscv32)

set(QEMU_FLAGS_${ARCH}
  -nographic
  -machine sifive_e
  )

board_set_flasher_ifnset(hifive1)
board_finalize_runner_args(hifive1)

board_runner_args(openocd --cmd-load "hifive1-load")
board_runner_args(openocd --cmd-reset-halt "hifive1-reset-halt")
board_runner_args(openocd --cmd-post-verify "hifive1-post-verify")

include(${ZEPHYR_BASE}/boards/common/openocd.board.cmake)
