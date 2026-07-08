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

if(CONFIG_INPUT_VIRTIO)
  set(QEMU_VIRTIO_INPUT_FLAGS -device virtio-tablet-device,bus=virtio-mmio-bus.3)
endif()

if(CONFIG_DISK_DRIVER_VIRTIO_BLK)
  # The matching "-drive ...,id=vblk" is added by cmake/emu/qemu.cmake; attach it
  # here so the MMIO virtio-blk device is backed by the disk image.
  set(virtio_blk_mmio_dev "virtio-blk-device,drive=vblk,bus=virtio-mmio-bus.4")
  set(blk_size ${CONFIG_QEMU_VIRTIO_BLK_LOGICAL_BLOCK_SIZE})
  # QEMU requires physical_block_size >= logical_block_size.
  set(virtio_blk_mmio_dev
    "${virtio_blk_mmio_dev},logical_block_size=${blk_size},physical_block_size=${blk_size}")
  set(QEMU_VIRTIO_BLK_FLAGS -device ${virtio_blk_mmio_dev})
endif()

set(QEMU_FLAGS_${ARCH}
  -cpu ${QEMU_CPU_TYPE_${ARCH}}
  ${QEMU_VIRTIO_ENTROPY_FLAGS}
  ${QEMU_VIRTIO_INPUT_FLAGS}
  ${QEMU_VIRTIO_BLK_FLAGS}
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
