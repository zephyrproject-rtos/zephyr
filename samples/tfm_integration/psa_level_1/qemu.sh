#!/bin/bash
#-----------------------------------------------------------------------------
# Copyright 2019 Linaro Limited
# SPDX-License-Identifier: Apache-2.0
#
# Merges a secure TF-M library that enables IPC communications with a
# non-secure Zephyr application.
#
# This files is a workaround since we can't currently insert a real post-build
# step in the Zephyr build system, so the zephyr and TF-M binaries need to
# be manually merged pending changes to the build system.
#-----------------------------------------------------------------------------

# Halt on errors
set -e
QEMUBOARD="mps2-an521"

# Merge bootloader and TFM image into single binary
srec_cat build/mcuboot.bin -Binary build/tfm_sign.bin -Binary -offset \
0x80000 -o tfm_qemu.bin -Binary

# Convert .bin to .elf with an appropriate offset
srec_cat tfm_qemu.bin -binary -offset 0x10000000 -o tfm_qemu.hex -intel \
--line-length=44

# Start qemu
echo "Starting QEMU (CTRL+C to quit)"
qemu-system-arm -M ${QEMUBOARD} -device loader,file=tfm_qemu.hex -serial stdio
