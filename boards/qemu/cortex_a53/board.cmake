# Copyright (c) 2019 Carlo Caione <ccaione@baylibre.com>
# SPDX-License-Identifier: Apache-2.0

set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_BINARY_SUFFIX aarch64)

set(QEMU_CPU_TYPE cortex-a53)

if(CONFIG_ARMV8_A_NS)
  set(QEMU_MACH virt,gic-version=3)
else()
  set(QEMU_MACH virt,secure=on,gic-version=3)
endif()

if(CONFIG_ENTROPY_VIRTIO)
  set(QEMU_VIRTIO_ENTROPY_FLAGS -device virtio-rng-device,bus=virtio-mmio-bus.0)
endif()

if(CONFIG_INPUT_VIRTIO)
  if(CONFIG_INPUT_VIRTIO_DEVICE_TYPE_KEYBOARD)
    set(QEMU_VIRTIO_INPUT_FLAGS -device virtio-keyboard-device,bus=virtio-mmio-bus.3)
  elseif(CONFIG_INPUT_VIRTIO_DEVICE_TYPE_TABLET)
    set(QEMU_VIRTIO_INPUT_FLAGS -device virtio-tablet-device,bus=virtio-mmio-bus.3)
  else()
    message(WARNING "No virtio input device type selected; QEMU_VIRTIO_INPUT_FLAGS will be empty")
  endif()
endif()

# MMIO transport the block device is attached to, matching the virtio_mmio node
# it hangs off in the board devicetree. The device itself is added by
# cmake/emu/qemu/virtio_blk.cmake.
set(QEMU_VIRTIO_BLK_TRANSPORT bus=virtio-mmio-bus.4)

set(QEMU_BOARD_FLAGS
  -cpu ${QEMU_CPU_TYPE}
  ${QEMU_VIRTIO_ENTROPY_FLAGS}
  ${QEMU_VIRTIO_INPUT_FLAGS}
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

include(${ZEPHYR_BASE}/boards/common/qemu.board.cmake)
