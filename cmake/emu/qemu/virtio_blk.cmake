# SPDX-FileCopyrightText: Copyright (c) 2026 Process Mission
# SPDX-License-Identifier: Apache-2.0

# Emulated virtio-blk disk, backed by a raw disk image created at build time.
#
# Boards say where the device goes by setting QEMU_VIRTIO_BLK_TRANSPORT to the
# device property picking their transport, "bus=virtio-mmio-bus.4" or
# "addr=06.0" for example, matching the devicetree.

if(CONFIG_DISK_DRIVER_VIRTIO_BLK)
  if(qemu_alternate_path)
    find_program(
      QEMU_IMG
      PATHS ${qemu_alternate_path}
      NO_DEFAULT_PATH
      NAMES qemu-img
    )
  else()
    find_program(
      QEMU_IMG
      qemu-img
    )
  endif()

  qemu_append_extra_flags(
    -drive file=${ZEPHYR_BINARY_DIR}/virtio_blk_disk.img,if=none,id=vblk,format=raw
  )

  if(CONFIG_VIRTIO_PCI)
    set(virtio_blk_dev "virtio-blk-pci,drive=vblk")
  else()
    set(virtio_blk_dev "virtio-blk-device,drive=vblk")
  endif()

  if(QEMU_VIRTIO_BLK_TRANSPORT)
    string(APPEND virtio_blk_dev ",${QEMU_VIRTIO_BLK_TRANSPORT}")
  endif()

  # QEMU requires physical_block_size >= logical_block_size.
  set(blk_size ${CONFIG_QEMU_VIRTIO_BLK_LOGICAL_BLOCK_SIZE})
  string(APPEND virtio_blk_dev
    ",logical_block_size=${blk_size},physical_block_size=${blk_size}")

  qemu_append_extra_flags(-device ${virtio_blk_dev})

  add_custom_target(qemu_virtio_blk_disk
    COMMAND
    ${CMAKE_COMMAND}
    -DQEMU_IMG=${QEMU_IMG}
    -DDISK_FILE=${ZEPHYR_BINARY_DIR}/virtio_blk_disk.img
    -DDISK_SIZE=${CONFIG_QEMU_VIRTIO_BLK_DISK_SIZE}
    -P ${ZEPHYR_BASE}/cmake/emu/qemu_virtio_blk_disk.cmake
  )

  qemu_add_target_depends(qemu_virtio_blk_disk)
endif()
