# SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
# SPDX-License-Identifier: Apache-2.0

# Locate the qemu-system-* binary matching the target architecture, and the
# UEFI firmware when booting through UEFI.
#
# A board that runs on a qemu-system-* whose name is not its ARCH sets
# QEMU_BINARY_SUFFIX; otherwise ARCH is used, with a few well known exceptions.
#
# The QEMU_BIN_PATH environment variable overrides the search entirely. Without
# it, a board whose QEMU ships outside PATH, typically bundled with its
# toolchain, lists the directories to try first in QEMU_BINARY_HINTS; PATH is
# searched after them.
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
    HINTS ${QEMU_BINARY_HINTS}
  )
endif()

# We need to set up uefi-run and OVMF environment
# for testing UEFI method on qemu platforms
if(CONFIG_QEMU_UEFI_BOOT AND CONFIG_X86_64)
  find_program(UEFI NAMES uefi-run REQUIRED)
  if(DEFINED ENV{OVMF_FD_PATH})
    set(OVMF_FD_PATH $ENV{OVMF_FD_PATH})
  else()
    message(FATAL_ERROR "Couldn't find an valid OVMF_FD_PATH.")
  endif()
  list(APPEND UEFI -b ${OVMF_FD_PATH} -q ${QEMU})
  set(QEMU ${UEFI})
elseif(CONFIG_QEMU_UEFI_BOOT AND CONFIG_ARM64)
  # EDK2 firmware for the virt machine: QEMU_EFI_FD_PATH if set, otherwise the
  # copy QEMU or the distribution ships
  if(DEFINED ENV{QEMU_EFI_FD_PATH})
    set(QEMU_EFI_FD_PATH $ENV{QEMU_EFI_FD_PATH})
  else()
    get_filename_component(qemu_bin_dir ${QEMU} DIRECTORY)
    find_file(QEMU_EFI_FD_PATH
      NAMES edk2-aarch64-code.fd QEMU_EFI.fd
      PATHS
        ${qemu_bin_dir}/../share/qemu
        ${qemu_bin_dir}/../xilinx/share/qemu-xilinx
        /usr/share/qemu
        /usr/share/qemu-efi-aarch64
        /usr/share/edk2/aarch64
      NO_DEFAULT_PATH
    )
    if(NOT QEMU_EFI_FD_PATH)
      message(FATAL_ERROR "No aarch64 UEFI firmware found, set QEMU_EFI_FD_PATH.")
    endif()
  endif()
endif()
