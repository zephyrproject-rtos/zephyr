# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Post-process the x86_64 ELF into the pair of images QEMU can boot directly.

if(CONFIG_X86_64 AND NOT CONFIG_QEMU_UEFI_BOOT)
  # QEMU doesn't like 64-bit ELF files. Since we don't use any >4GB
  # addresses, converting it to 32-bit is safe enough for emulation.
  add_custom_target(qemu_image_target
    COMMAND
    ${CMAKE_OBJCOPY}
    -O elf32-i386
    $<TARGET_FILE:${logical_target_for_zephyr_elf}>
    ${ZEPHYR_BINARY_DIR}/zephyr-qemu.elf
    DEPENDS ${logical_target_for_zephyr_elf}
  )

  # Split the 'locore' and 'main' memory regions into separate executable
  # images and specify the 'locore' as the boot kernel, in order to prevent
  # the QEMU direct multiboot kernel loader from overwriting the BIOS and
  # option ROM areas located in between the two memory regions.
  # (for more details, refer to the issue zephyrproject-rtos/sdk-ng#168)
  add_custom_target(qemu_locore_image_target
    COMMAND
    ${CMAKE_OBJCOPY}
    -j .locore
    ${ZEPHYR_BINARY_DIR}/zephyr-qemu.elf
    ${ZEPHYR_BINARY_DIR}/zephyr-qemu-locore.elf
    2>&1 | grep -iv \"empty loadable segment detected\" || true
    DEPENDS qemu_image_target
  )

  add_custom_target(qemu_main_image_target
    COMMAND
    ${CMAKE_OBJCOPY}
    -R .locore
    ${ZEPHYR_BINARY_DIR}/zephyr-qemu.elf
    ${ZEPHYR_BINARY_DIR}/zephyr-qemu-main.elf
    2>&1 | grep -iv \"empty loadable segment detected\" || true
    DEPENDS qemu_image_target
  )

  add_custom_target(
    qemu_kernel_target
    DEPENDS qemu_locore_image_target qemu_main_image_target
  )

  list(APPEND QEMU_TARGET_DEPENDS qemu_kernel_target)

  set(QEMU_KERNEL_FILE "${ZEPHYR_BINARY_DIR}/zephyr-qemu-locore.elf")

  list(APPEND QEMU_EXTRA_FLAGS
    "-device;loader,file=${ZEPHYR_BINARY_DIR}/zephyr-qemu-main.elf"
  )
endif()
