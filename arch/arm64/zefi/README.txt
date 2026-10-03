SPDX-License-Identifier: Apache-2.0
Copyright (c) 2026 KylinSoft Corporation

AArch64 EFI wrapper
===================

Operation
=========

Enable CONFIG_BUILD_OUTPUT_EFI on a supported AArch64 board. The build emits
zephyr.efi alongside zephyr.elf. Copy zephyr.efi to the target's EFI System
Partition and start it from the UEFI shell.

Theory
======

UEFI applications are relocatable PE/COFF images and firmware may load them at
an arbitrary address. zefi.py creates such an image by combining a small
AArch64 EFI stub with the loadable segments extracted from zephyr.elf.

At startup, the stub copies those segments to the addresses for which Zephyr
was linked, clears zero-filled areas such as BSS, synchronizes the data and
instruction caches, disables the MMU at EL1 or EL2, and branches to __start.

QEMU + AAVMF
============

qemu_cortex_a53 can start zephyr.efi through AAVMF when
CONFIG_QEMU_UEFI_BOOT is enabled. On Debian and Ubuntu, install:

  qemu-system-arm qemu-efi-aarch64 parted dosfstools mtools

The default firmware files are:

  /usr/share/AAVMF/AAVMF_CODE.fd
  /usr/share/AAVMF/AAVMF_VARS.fd

Set OVMF_FD_PATH and AAVMF_VARS_PATH to use files installed elsewhere.

The QEMU launcher creates an MBR-partitioned FAT32 disk image containing
EFI/BOOT/BOOTAA64.EFI. parted is invoked non-interactively on that regular
image file in the build directory; it is never given a host block device.

AAVMF runs in the non-secure world, so qemu_cortex_a53 must use
CONFIG_ARMV8_A_NS=y.

Source Code and Linkage
=======================

The EFI stub is linked as a freestanding, position-independent ELF image and
then converted to PE/COFF. Except for efi_entry, its functions and objects must
have internal linkage so that the compiler can generate position-independent
references without relying on ELF dynamic relocation.

The Zephyr SDK AArch64 toolchain pulls in crt0 when linking with -shared.
Therefore, the stub uses -nostdlib and -static, with efi_entry selected as the
entry point.
