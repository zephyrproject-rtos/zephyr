# SPDX-License-Identifier: Apache-2.0
# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

set(SUPPORTED_EMU_PLATFORMS qemu)
set(QEMU_ARCH hexagon)
set(QEMU_binary_suffix hexagon)

set(QEMU_FLAGS_${ARCH}
  -machine virt
  -m 4G
)

# Hexagon boots Zephyr as an H2 hypervisor guest.  QEMU loads H2's
# "loadlinux" as -bios; H2 then boots the Zephyr ELF placed at the
# guest load address via -device loader.
#
# Set HEXAGON_H2_LOADLINUX (env var or cmake -D) to override the
# default search path for the H2 loadlinux boot loader.
if(DEFINED ENV{HEXAGON_H2_LOADLINUX})
  set(HEXAGON_H2_LOADLINUX $ENV{HEXAGON_H2_LOADLINUX})
endif()

if(NOT HEXAGON_H2_LOADLINUX)
  set(HEXAGON_H2_LOADLINUX "${ZEPHYR_BASE}/hexagon-hypervisor/linux/loadlinux")
endif()

get_filename_component(HEXAGON_H2_LOADLINUX "${HEXAGON_H2_LOADLINUX}" ABSOLUTE)

if(NOT EXISTS "${HEXAGON_H2_LOADLINUX}")
  message(WARNING
    "H2 loadlinux not found at: ${HEXAGON_H2_LOADLINUX}\n"
    "Hexagon requires the H2 hypervisor boot loader to run under QEMU.\n"
    "Build-only mode will work, but 'west build -t run' will fail.\n"
    "Set HEXAGON_H2_LOADLINUX (env var or -DHEXAGON_H2_LOADLINUX=<path>) "
    "to the H2 loadlinux binary.\n"
    "See: https://github.com/qualcomm/hexagon-hypervisor"
  )
endif()

# Use loadlinux as the QEMU BIOS and pass zephyr.elf via device loader.
# The generic loader parses ELF program headers and places segments at their
# physical addresses, so no explicit addr= or objcopy-to-bin is needed.
set(QEMU_KERNEL_OPTION "-bios;${HEXAGON_H2_LOADLINUX}")

list(APPEND QEMU_EXTRA_FLAGS
  "-device;loader,file=${ZEPHYR_BINARY_DIR}/${KERNEL_ELF_NAME}"
)

board_set_debugger_ifnset(qemu)
