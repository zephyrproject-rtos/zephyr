# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Locate the qemu-system-* binary matching the target architecture, and wrap it
# in uefi-run when booting through UEFI.
#
# A board that runs on a qemu-system-* whose name is not its ARCH sets
# QEMU_BINARY_SUFFIX; otherwise ARCH is used, with a few well known exceptions.
#
# Sets: QEMU_BINARY_SUFFIX, QEMU, qemu_alternate_path

if(DEFINED QEMU_ARCH)
  message(FATAL_ERROR
    "QEMU_ARCH was only ever an alias for the qemu-system-* suffix and has "
    "been removed. Set QEMU_BINARY_SUFFIX instead."
  )
endif()

if(DEFINED QEMU_binary_suffix)
  message(FATAL_ERROR "QEMU_binary_suffix has been renamed to QEMU_BINARY_SUFFIX.")
endif()

if("${ARCH}" STREQUAL "x86")
  set_ifndef(QEMU_BINARY_SUFFIX i386)
elseif("${ARCH}" STREQUAL "mips")
  if(CONFIG_BIG_ENDIAN)
    set_ifndef(QEMU_BINARY_SUFFIX mips)
  else()
    set_ifndef(QEMU_BINARY_SUFFIX mipsel)
  endif()
elseif("${ARCH}" STREQUAL "openrisc")
  set_ifndef(QEMU_BINARY_SUFFIX or1k)
else()
  set_ifndef(QEMU_BINARY_SUFFIX ${ARCH})
endif()

set(qemu_alternate_path $ENV{QEMU_BIN_PATH})
if(qemu_alternate_path)
  find_program(
    QEMU
    PATHS ${qemu_alternate_path}
    NO_DEFAULT_PATH
    NAMES qemu-system-${QEMU_BINARY_SUFFIX}
  )
else()
  find_program(
    QEMU
    qemu-system-${QEMU_BINARY_SUFFIX}
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
