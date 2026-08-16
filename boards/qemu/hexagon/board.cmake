# SPDX-License-Identifier: Apache-2.0
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_BINARY_SUFFIX hexagon)

set(QEMU_BOARD_FLAGS
  -machine virt
  -m 4G
)

# Hexagon boots Zephyr as an H2 hypervisor guest.  QEMU loads H2's
# "loadlinux" as -bios; H2 then boots the Zephyr ELF placed at the
# guest load address via -device loader.
#
# The Hexagon LLVM cross-toolchain (clang+llvm-*-cross-hexagon-*) ships its
# own hexagon_loadlinux_v* firmware under <toolchain>/share/qemu/.
if(DEFINED ENV{HEXAGON_H2_LOADLINUX})
  set(HEXAGON_H2_LOADLINUX $ENV{HEXAGON_H2_LOADLINUX})
endif()

if(HEXAGON_H2_LOADLINUX)
  get_filename_component(HEXAGON_H2_LOADLINUX "${HEXAGON_H2_LOADLINUX}" ABSOLUTE)

  if(NOT EXISTS "${HEXAGON_H2_LOADLINUX}")
    message(WARNING
      "HEXAGON_H2_LOADLINUX set but not found at: ${HEXAGON_H2_LOADLINUX}\n"
      "Falling back to QEMU's bundled H2 loadlinux firmware."
    )
    unset(HEXAGON_H2_LOADLINUX)
  endif()
endif()

if(HEXAGON_H2_LOADLINUX)
  set(QEMU_KERNEL_OPTION "-bios;${HEXAGON_H2_LOADLINUX}")
else()
  set(QEMU_KERNEL_OPTION "")
endif()

list(APPEND QEMU_EXTRA_FLAGS
  "-device;loader,file=${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME}"
)

board_set_debugger_ifnset(qemu)
