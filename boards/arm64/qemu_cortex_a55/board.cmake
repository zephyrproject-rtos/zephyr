# Copyright 2022 NXP
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_ARCH aarch64)

set(QEMU_CPU_TYPE_${ARCH} cortex-a55)

if(CONFIG_ARMV8_A_NS)
set(QEMU_MACH virt,gic-version=3)
else()
set(QEMU_MACH virt,secure=on,gic-version=3)
endif()

set(QEMU_FLAGS_${ARCH}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  -nographic
  -machine ${QEMU_MACH}
  )

if(CONFIG_XIP)
  set(QEMU_KERNEL_OPTION
  -bios ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
  )
endif()

board_set_debugger_ifnset(qemu)
