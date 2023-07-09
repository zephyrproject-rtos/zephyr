# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

if(CONFIG_BOARD_QEMU_XTENSA OR CONFIG_BOARD_QEMU_XTENSA_MMU)
  set(QEMU_CPU_TYPE_${ARCH} dc233c)

  set(QEMU_FLAGS_${ARCH}
    -machine sim -semihosting -nographic -cpu dc233c
  )
endif()

board_runner_args(qemu "--commander=qemu-system-xtensa" "--machine=sim" "--cpu=sample_controller"
  "--qemu_option=${CMAKE_CURRENT_LIST_DIR}/qemu_xtensa_option.yml")

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
