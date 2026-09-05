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

# Wrap QEMU in a UEFI launcher. x86_64 uses uefi-run + OVMF; AArch64 uses
# arch/arm64/zefi/uefi_run.py + AAVMF (partitioned FAT ESP).
if(CONFIG_QEMU_UEFI_BOOT)
  if(CONFIG_ARM64)
    set(UEFI ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/arch/arm64/zefi/uefi_run.py)
    if(DEFINED ENV{OVMF_FD_PATH})
      set(OVMF_FD_PATH $ENV{OVMF_FD_PATH})
    elseif(EXISTS /usr/share/AAVMF/AAVMF_CODE.fd)
      set(OVMF_FD_PATH /usr/share/AAVMF/AAVMF_CODE.fd)
    else()
      message(FATAL_ERROR
        "Couldn't find a valid OVMF_FD_PATH. Set OVMF_FD_PATH or install "
        "qemu-efi-aarch64 (AAVMF_CODE.fd)."
      )
    endif()
    if(DEFINED ENV{AAVMF_VARS_PATH})
      set(AAVMF_VARS_PATH $ENV{AAVMF_VARS_PATH})
    else()
      set(AAVMF_VARS_PATH /usr/share/AAVMF/AAVMF_VARS.fd)
    endif()
    if(NOT EXISTS ${AAVMF_VARS_PATH})
      message(FATAL_ERROR
        "Couldn't find AAVMF vars at ${AAVMF_VARS_PATH}. "
        "Install qemu-efi-aarch64 or set AAVMF_VARS_PATH."
      )
    endif()
    find_program(ZEFI_PARTED NAMES parted)
    find_program(ZEFI_MKFS_VFAT NAMES mkfs.vfat)
    find_program(ZEFI_MMD NAMES mmd)
    find_program(ZEFI_MCOPY NAMES mcopy)
    if(NOT ZEFI_PARTED OR NOT ZEFI_MKFS_VFAT OR NOT ZEFI_MMD OR NOT ZEFI_MCOPY)
      message(FATAL_ERROR
        "ARM64 QEMU UEFI boot needs host tools: parted, mkfs.vfat (dosfstools), "
        "mmd and mcopy (mtools)."
      )
    endif()
  else()
    find_program(UEFI NAMES uefi-run REQUIRED)
    if(DEFINED ENV{OVMF_FD_PATH})
      set(OVMF_FD_PATH $ENV{OVMF_FD_PATH})
    else()
      message(FATAL_ERROR "Couldn't find an valid OVMF_FD_PATH.")
    endif()
  endif()
  list(APPEND UEFI -b ${OVMF_FD_PATH} -q ${QEMU})
  set(QEMU ${UEFI})
endif()
