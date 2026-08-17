# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Emulated NVMe controller, backed by a disk image created at build time.

if(CONFIG_NVME)
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
    -drive file=${ZEPHYR_BINARY_DIR}/nvme_disk.img,if=none,id=nvm1
    -device nvme,serial=deadbeef,drive=nvm1
  )

  add_custom_target(qemu_nvme_disk
    COMMAND
    ${QEMU_IMG}
    create
    ${ZEPHYR_BINARY_DIR}/nvme_disk.img
    1M
  )

  qemu_add_target_depends(qemu_nvme_disk)
endif()
