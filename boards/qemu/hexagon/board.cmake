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
# "loadlinux" as -kernel; H2 then boots the raw Zephyr binary placed
# at the guest load address via -device loader.
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
    "Set HEXAGON_H2_LOADLINUX to the H2 boot loader path "
    "to enable QEMU emulation."
  )
endif()

# Use loadlinux as the QEMU kernel and pass zephyr.bin via device loader.
# The build system generates zephyr.bin automatically (CONFIG_BUILD_OUTPUT_BIN).
set(QEMU_KERNEL_OPTION "-kernel;${HEXAGON_H2_LOADLINUX}")

list(APPEND QEMU_EXTRA_FLAGS
  "-device;loader,addr=0xa0000000,file=${ZEPHYR_BINARY_DIR}/${KERNEL_BIN_NAME}"
)

board_set_debugger_ifnset(qemu)
