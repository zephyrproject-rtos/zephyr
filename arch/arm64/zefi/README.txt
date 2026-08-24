SPDX-License-Identifier: Apache-2.0
Copyright (c) 2026 The Zephyr Project Contributors

AArch64 EFI wrapper
===================

This directory mirrors arch/x86/zefi for ARM64. When CONFIG_ARM64_EFI is
enabled, zefi.py wraps zephyr.elf as a PE/COFF EFI application (zephyr.efi).

The firmware loads zephyr.efi at an arbitrary address. The stub copies the
embedded PT_LOAD segments to their linked addresses, zeros BSS, cleans
data/instruction caches, disables the MMU at EL1 or EL2, and branches to
__start.

Build
=====

Enable CONFIG_BUILD_OUTPUT_EFI and CONFIG_ARM64_EFI on an ARM64 board (for
example qemu_cortex_a53). The board CMakeLists.txt invokes zefi.py after the
ELF is linked.

QEMU + AAVMF
============

CONFIG_QEMU_UEFI_BOOT uses uefi_run.py instead of the x86 uefi-run/OVMF path.
AAVMF needs a MBR-partitioned FAT32 ESP; the helper builds one with mtools.

CMake (when CONFIG_QEMU_UEFI_BOOT=y on ARM64) and uefi_run.py both require:

  - qemu-efi-aarch64: AAVMF_CODE.fd and AAVMF_VARS.fd
  - parted
  - dosfstools (mkfs.vfat)
  - mtools (mmd, mcopy)

Host packages (Debian/Ubuntu names):

  qemu-system-arm qemu-efi-aarch64 parted dosfstools mtools

Firmware: /usr/share/AAVMF/AAVMF_CODE.fd (or set OVMF_FD_PATH).
Vars template: /usr/share/AAVMF/AAVMF_VARS.fd (or set AAVMF_VARS_PATH).

AAVMF is non-secure UEFI. qemu_cortex_a53 must use CONFIG_ARMV8_A_NS=y
(secure=on virt has no serial output with this virtio ESP).

Linkage notes
=============

Same PE/COFF constraints as x86 (see arch/x86/zefi/README.txt): keep the stub
in a single C file with static symbols except efi_entry.

AArch64-gcc in the Zephyr SDK pulls crt0 when linking -shared, so the stub is
linked freestanding (-nostdlib -static -Wl,-e,efi_entry) rather than -shared.
