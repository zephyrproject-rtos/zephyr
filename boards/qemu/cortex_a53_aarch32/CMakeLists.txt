# Copyright (c) 2026 Realtek Semiconductor Corp.
# SPDX-License-Identifier: Apache-2.0

# Build the AArch64 EL3 boot shim that downgrades the CPU to AArch32 EL1 and
# enters Zephyr (see shim/shim.S). The shim is a separate boot image with a
# different ELF machine type than Zephyr, so it must be built out-of-band with
# the SDK's 64-bit aarch64-zephyr-elf toolchain rather than this board's 32-bit
# arm-zephyr-eabi Zephyr toolchain. The commands run as a post-build step
# producing shim/shim.bin, which board.cmake passes to QEMU via -bios.

set(SHIM_SRC ${CMAKE_CURRENT_LIST_DIR}/shim)
set(SHIM_BUILD ${CMAKE_BINARY_DIR}/shim)
file(MAKE_DIRECTORY ${SHIM_BUILD})

# Reuse the toolchain prefix Zephyr resolved for this board (CROSS_COMPILE),
# swapping the 32-bit target triple for the 64-bit one the shim is built with.
string(REPLACE "arm-zephyr-eabi" "aarch64-zephyr-elf" SHIM_TOOLCHAIN "${CROSS_COMPILE}")

if(NOT EXISTS ${SHIM_TOOLCHAIN}gcc)
  message(FATAL_ERROR
    "AArch64 shim compiler not found at '${SHIM_TOOLCHAIN}gcc'. This board "
    "boots via an AArch64 EL3 shim and requires the Zephyr SDK's "
    "aarch64-zephyr-elf toolchain.")
endif()

set_property(GLOBAL APPEND PROPERTY extra_post_build_commands
  COMMAND ${SHIM_TOOLCHAIN}gcc -x assembler-with-cpp -nostdlib -c
          ${SHIM_SRC}/shim.S -o ${SHIM_BUILD}/shim.o
  COMMAND ${SHIM_TOOLCHAIN}ld -T ${SHIM_SRC}/shim.ld
          ${SHIM_BUILD}/shim.o -o ${SHIM_BUILD}/shim.elf
  COMMAND ${SHIM_TOOLCHAIN}objcopy -O binary
          ${SHIM_BUILD}/shim.elf ${SHIM_BUILD}/shim.bin
  )
