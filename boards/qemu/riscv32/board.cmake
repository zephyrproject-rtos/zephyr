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

if(CONFIG_QEMU_ENTRY_WRAPPER)
  find_program(QEMU_BINARY qemu-system-${QEMU_BINARY_SUFFIX} REQUIRED)
  set(QEMU
    ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/scripts/utils/qemu_entry_wrapper.py
    ${QEMU_BINARY}
    ${CONFIG_MP_MAX_NUM_CPUS}
    )
endif()

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
