/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/riscv/irq.h>
#include <zephyr/offsets.h>
#include <zephyr/toolchain.h>

#define T153_MINTSTATUS 0x346
#define T153_MCAUSE_MPIL 0x00ff0000
#define T153_MEXSTATUS 0x7e1
#define T153_MEXSTATUS_EXPT_VLD (1 << 5)

GTEXT(__soc_save_context)
GTEXT(__soc_restore_context)

SECTION_FUNC(exception.other, __soc_save_context)
	/* Synchronous exceptions do not copy the current level into MPIL. */
	csrr t0, T153_MINTSTATUS
	sw t0, __soc_esf_t_mintstatus_OFFSET(a0)
	ret

SECTION_FUNC(exception.other, __soc_restore_context)
	addi a1, a0, -__struct_arch_esf_soc_context_OFFSET
	lw t0, __struct_arch_esf_mcause_OFFSET(a1)
	bltz t0, 1f
	lw t1, __soc_esf_t_mintstatus_OFFSET(a0)
	/* Move MIL[31:24] into the saved MPIL[23:16] field. */
	srli t1, t1, 8
	li t2, T153_MCAUSE_MPIL
	and t1, t1, t2
	not t2, t2
	and t0, t0, t2
	or t0, t0, t1
1:
	/*
	 * Scheduling can resume a frame saved by a different kind of trap.
	 * Match the return type to the active hardware trap: IRQ returns
	 * reload MIL, while exception returns must clear EXPT_VLD.
	 */
	csrr t1, T153_MEXSTATUS
	andi t1, t1, T153_MEXSTATUS_EXPT_VLD
	li t2, RISCV_MCAUSE_IRQ_BIT
	bnez t1, 2f
	/* An interrupt return must reload MIL from the resumed frame's MPIL. */
	or t0, t0, t2
	j 3f
2:
	/* An exception return must clear EXPT_VLD so later exceptions work. */
	not t2, t2
	and t0, t0, t2
3:
	sw t0, __struct_arch_esf_mcause_OFFSET(a1)
	ret
