# Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
# Copyright (c) 2022 Huawei France Technologies SASU
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_ARCH aarch64)

set(QEMU_MACH virt,gic-version=3,accel=kvm)

if(NOT CONFIG_ARMV8_A_NS)
  set(QEMU_MACH ${QEMU_MACH},secure=on)
endif()

set(QEMU_FLAGS_${ARCH}
  -cpu host
  -nographic
  -machine ${QEMU_MACH}
  )

if(CONFIG_XIP)
  # This should be equivalent to
  #   ... -drive if=pflash,file=build/zephyr/zephyr.bin,format=raw
  # without having to pad the binary file to the FLASH size
  set(QEMU_KERNEL_OPTION
  -bios ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
  )
endif()

board_set_debugger_ifnset(qemu)
