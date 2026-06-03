/* SPDX-License-Identifier: Apache-2.0 */
/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. */
/*
 * Trampoline for qemu_riscv64/smp_large: QEMU virt resets to VIRT_DRAM
 * (0x80000000) regardless of the ELF entry point.  This stub is placed
 * at 0x80000000 and performs an absolute jump to the real Zephyr entry
 * at 0x100000000 using the large code model.
 */

#include <zephyr/toolchain.h>

SECTION_FUNC(trampoline, _trampoline_to_far)
	.option push
	.option norelax
	/* Load 0x100000000 into t0 and jump there */
	li	t0, 0x100000000
	jr	t0
	.option pop
