/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/linker/sections.h>
#include <zephyr/toolchain.h>

#include "asm_macros.inc"

GTEXT(z_riscv_debugpoint_insn_read)
GTEXT(z_riscv_debugpoint_insn_read_fault_start)
GTEXT(z_riscv_debugpoint_insn_read_fault_end)
GTEXT(z_riscv_debugpoint_insn_read_fixup)
GTEXT(z_riscv_debugpoint_tinfo_read)
GTEXT(z_riscv_debugpoint_tinfo_read_fault_start)
GTEXT(z_riscv_debugpoint_tinfo_read_fault_end)
GTEXT(z_riscv_debugpoint_tinfo_read_fixup)

/*
 * int z_riscv_debugpoint_insn_read(const void *pc, uint32_t *instruction)
 */
SECTION_FUNC(TEXT, z_riscv_debugpoint_insn_read)
	li	a2, 0

z_riscv_debugpoint_insn_read_fault_start:
	lhu	a3, 0(a0)
	andi	a4, a3, 3
	li	a5, 3
	bne	a4, a5, 1f
	lhu	a2, 2(a0)
1:
z_riscv_debugpoint_insn_read_fault_end:
	slli	a2, a2, 16
	or	a3, a3, a2
	sw	a3, 0(a1)
	li	a0, 0
	ret

z_riscv_debugpoint_insn_read_fixup:
	li	a0, -1
	ret

/*
 * int z_riscv_debugpoint_tinfo_read(uintptr_t *value)
 */
SECTION_FUNC(TEXT, z_riscv_debugpoint_tinfo_read)
z_riscv_debugpoint_tinfo_read_fault_start:
	csrr	a1, 0x7a4
z_riscv_debugpoint_tinfo_read_fault_end:
	sr	a1, 0(a0)
	li	a0, 0
	ret

z_riscv_debugpoint_tinfo_read_fixup:
	li	a0, -1
	ret
