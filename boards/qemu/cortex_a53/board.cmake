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

if(CONFIG_ENTROPY_VIRTIO)
  set(QEMU_VIRTIO_ENTROPY_FLAGS -device virtio-rng-device,bus=virtio-mmio-bus.0)
endif()

set(QEMU_FLAGS_${ARCH}
  -global virtio-mmio.force-legacy=false
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  ${QEMU_VIRTIO_ENTROPY_FLAGS}
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
