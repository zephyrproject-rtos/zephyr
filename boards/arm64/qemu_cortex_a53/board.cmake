# Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_ARCH aarch64)

set(QEMU_CPU_TYPE_${ARCH} cortex-a53)

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
  # This should be equivalent to
  #   ... -drive if=pflash,file=build/zephyr/zephyr.bin,format=raw
  # without having to pad the binary file to the FLASH size
  set(QEMU_KERNEL_OPTION
  -bios ${PROJECT_BINARY_DIR}/${CONFIG_KERNEL_BIN_NAME}.bin
  )
  board_runner_args(qemu "--commander=qemu-system-aarch64" "--cpu=cortex-a53" "--machine=${QEMU_MACH}"
              "--qemu_option=${CMAKE_CURRENT_LIST_DIR}/qemu_cortex_a53_xip_option.yml")
else()
  board_runner_args(qemu "--commander=qemu-system-aarch64" "--cpu=cortex-a53" "--machine=${QEMU_MACH}")
endif()

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
