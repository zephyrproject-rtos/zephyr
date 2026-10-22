# SPDX-License-Identifier: Apache-2.0
set(SUPPORTED_EMU_PLATFORMS qemu)

set(QEMU_BINARY_SUFFIX riscv32)
set(QEMU_CPU_TYPE riscv32)

set(QEMU_BOARD_FLAGS
  -machine sifive_e
)

if(CONFIG_QEMU_DEVICE_LOADER)
  set(QEMU_KERNEL_OPTION "")
  math(EXPR max_cpu_index "${CONFIG_MP_MAX_NUM_CPUS} - 1")
  foreach(cpu_num RANGE 0 ${max_cpu_index})
    list(APPEND QEMU_KERNEL_OPTION
      "-device;loader,file=\$<TARGET_FILE:\${logical_target_for_zephyr_elf}>,cpu-num=${cpu_num}"
    )
  endforeach()
endif()

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
