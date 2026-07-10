# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Locate the qemu-system-* binary matching the target architecture, and wrap it
# in uefi-run when booting through UEFI.
#
# Sets: QEMU_binary_suffix, QEMU, qemu_alternate_path

if("${ARCH}" STREQUAL "x86")
  set_ifndef(QEMU_binary_suffix i386)
elseif("${ARCH}" STREQUAL "mips")
  if(CONFIG_BIG_ENDIAN)
    set_ifndef(QEMU_binary_suffix mips)
  else()
    set_ifndef(QEMU_binary_suffix mipsel)
  endif()
elseif("${ARCH}" STREQUAL "openrisc")
  set_ifndef(QEMU_binary_suffix or1k)
elseif(DEFINED QEMU_ARCH)
  set_ifndef(QEMU_binary_suffix ${QEMU_ARCH})
else()
  set_ifndef(QEMU_binary_suffix ${ARCH})
endif()

set(qemu_alternate_path $ENV{QEMU_BIN_PATH})
if(qemu_alternate_path)
  find_program(
    QEMU
    PATHS ${qemu_alternate_path}
    NO_DEFAULT_PATH
    NAMES qemu-system-${QEMU_binary_suffix}
  )
else()
  find_program(
    QEMU
    qemu-system-${QEMU_binary_suffix}
  )
endif()

# We need to set up uefi-run and OVMF environment
# for testing UEFI method on qemu platforms
if(CONFIG_QEMU_UEFI_BOOT)
  find_program(UEFI NAMES uefi-run REQUIRED)
  if(DEFINED ENV{OVMF_FD_PATH})
    set(OVMF_FD_PATH $ENV{OVMF_FD_PATH})
  else()
    message(FATAL_ERROR "Couldn't find an valid OVMF_FD_PATH.")
  endif()
  list(APPEND UEFI -b ${OVMF_FD_PATH} -q ${QEMU})
  set(QEMU ${UEFI})
endif()
