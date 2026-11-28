# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

if(CONFIG_QEMU_CORTEX_A53_EXECUTION_STATE_AARCH32)
  # Build the AArch64 EL3 boot shim that downgrades the CPU to AArch32 EL1 and
  # enters Zephyr (see shim/shim.S). The shim is a separate boot image with a
  # different ELF machine type than Zephyr, so it must be built out-of-band with
  # the SDK's 64-bit aarch64-zephyr-elf toolchain rather than this board's 32-bit
  # arm-zephyr-eabi Zephyr toolchain. The commands run as a post-build step
  # producing shim/shim.bin, which board.cmake passes to QEMU via -bios.

  set(shim_src ${CMAKE_CURRENT_LIST_DIR}/shim)
  set(shim_build ${CMAKE_BINARY_DIR}/shim)
  file(MAKE_DIRECTORY ${shim_build})

  dt_chosen(zephyr_sram_path PROPERTY "zephyr,sram")
  dt_reg_addr(zephyr_load_address PATH "${zephyr_sram_path}")

  # Reuse the toolchain prefix Zephyr resolved for this board (CROSS_COMPILE),
  # swapping the 32-bit target triple for the 64-bit one the shim is built with.
  string(REPLACE "arm-zephyr-eabi" "aarch64-zephyr-elf" shim_toolchain "${CROSS_COMPILE}")

  if(NOT EXISTS ${shim_toolchain}gcc)
    message(FATAL_ERROR
      "AArch64 shim compiler not found at '${shim_toolchain}gcc'. This target "
      "boots via an AArch64 EL3 shim and requires the Zephyr SDK's "
      "aarch64-zephyr-elf toolchain.")
  endif()

  set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
    COMMAND ${shim_toolchain}gcc -nostdlib -c
            -DZEPHYR_LOAD_ADDRESS=${zephyr_load_address}
            ${shim_src}/shim.S -o ${shim_build}/shim.o
    COMMAND ${shim_toolchain}ld -T ${shim_src}/shim.ld
            ${shim_build}/shim.o -o ${shim_build}/shim.elf
    COMMAND ${shim_toolchain}objcopy -O binary
            ${shim_build}/shim.elf ${shim_build}/shim.bin
  )
endif()
