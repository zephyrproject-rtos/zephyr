# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)

include(${ZEPHYR_BASE}/boards/common/qemu_riscv.board.cmake)

qemu_riscv_cpu_from_dt(qemu_riscv_cpu)
qemu_riscv_binary_suffix(QEMU_BINARY_SUFFIX)

set(QEMU_CPU_TYPE "${qemu_riscv_cpu}")

if(CONFIG_RISCV_APLIC_MSI)
  set(QEMU_MACH virt,aia=aplic-imsic)
elseif(CONFIG_RISCV_APLIC_DIRECT)
  set(QEMU_MACH virt,aia=aplic)
else()
  set(QEMU_MACH virt)
endif()

set(QEMU_BOARD_FLAGS
  -machine ${QEMU_MACH}
  -bios none
  -m 256
  -cpu ${qemu_riscv_cpu}
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
