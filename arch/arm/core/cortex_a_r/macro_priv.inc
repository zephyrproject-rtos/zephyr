/*
 * Copyright (c) 2023 Arm Limited (or its affiliates). All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _MACRO_PRIV_INC_
#define _MACRO_PRIV_INC_

#include <zephyr/arch/arm/cortex_a_r/tpidruro.h>

/*
 * Get CPU id
 */

.macro get_cpu_id rreg0
	/* Read MPIDR register */
	mrc p15, 0, \rreg0, c0, c0, 5
	ubfx	\rreg0, \rreg0, #0, #24
.endm

/*
 * Get CPU logic id by looking up cpu_node_list
 * returns
 *   reg0: MPID
 *   reg1: logic id (0 ~ CONFIG_MP_MAX_NUM_CPUS - 1)
 * clobbers: reg0, reg1, reg2, reg3
 */
.macro get_cpu_logic_id reg0, reg1, reg2, reg3
	get_cpu_id \reg0
	ldr	\reg3, =cpu_node_list
	mov	\reg1, #0
1:	ldr	\reg2, [\reg3, \reg1, lsl #2]
	cmp	\reg2, \reg0
	beq	2f
	add	\reg1, \reg1, #1
	cmp	\reg1, #CONFIG_MP_MAX_NUM_CPUS
	bne	1b
	b	.
2:
.endm

.macro get_cpu rreg0
	/*
         * Get CPU pointer.
	 */
	mrc p15, 0, \rreg0, c13, c0, 3
        and \rreg0, #TPIDRURO_CURR_CPU
.endm

/*
 * DWARF for GDB at an AArch32 abort vector (no ESF yet).
 * LR_abt/und is faulting PC plus 8 (data) or 4 (prefetch), not a C RA.
 * SP is the exception-mode stack; previous SP is restored after exception_entry.
 *
 * DW_CFA_val_expression r15, DW_OP_breg14 -N  =>  PC = LR - N
 */
.macro cfi_aarch32_dabt_frame
	.cfi_signal_frame
	.cfi_return_column pc
	.cfi_undefined sp
	/* DW_CFA_val_expression: PC = LR - 8.
	 * r15, expression length 2, DW_OP_breg14, SLEB128(-8).
	 */
	.cfi_escape 0x16, 0x0f, 0x02, 0x7e, 0x78
.endm

.macro cfi_aarch32_pabt_frame
	.cfi_signal_frame
	.cfi_return_column pc
	.cfi_undefined sp
	/* DW_CFA_val_expression: PC = LR - 4.
	 * r15, expression length 2, DW_OP_breg14, SLEB128(-4).
	 */
	.cfi_escape 0x16, 0x0f, 0x02, 0x7e, 0x7c
.endm

/*
 * Saved GPR locations in __basic_sf, as byte offsets from the CFA.
 * above is the number of frame bytes between the end of __basic_sf and the
 * CFA (4 for the interrupted SP word on the non-switch path).
 */
.macro cfi_basic_sf_offsets above=0
	.cfi_offset pc, (___basic_sf_t_pc_OFFSET - ___basic_sf_t_SIZEOF - \above)
	.cfi_offset lr, (___basic_sf_t_lr_OFFSET - ___basic_sf_t_SIZEOF - \above)
	.cfi_offset r12, (___basic_sf_t_r12_OFFSET - ___basic_sf_t_SIZEOF - \above)
	.cfi_offset r3, (___basic_sf_t_r3_OFFSET - ___basic_sf_t_SIZEOF - \above)
	.cfi_offset r2, (___basic_sf_t_r2_OFFSET - ___basic_sf_t_SIZEOF - \above)
	.cfi_offset r1, (___basic_sf_t_r1_OFFSET - ___basic_sf_t_SIZEOF - \above)
	.cfi_offset r0, (___basic_sf_t_r0_OFFSET - ___basic_sf_t_SIZEOF - \above)
.endm

#if defined(CONFIG_FPU_SHARING)
#define CFI_IRQ_FPU_SIZE ___fpu_t_SIZEOF
#else
#define CFI_IRQ_FPU_SIZE 0
#endif

#define CFI_IRQ_CFA_OFFSET (CFI_IRQ_FPU_SIZE + ___basic_sf_t_SIZEOF)

/*
 * ISR runs on the SVC stack; the thread iframe SP was just pushed on that
 * stack (see _isr_wrapper). CFA = *svc_sp + CFI_IRQ_CFA_OFFSET.
 *
 * DW_CFA_def_cfa_expression: DW_OP_breg13+0, DW_OP_deref, DW_OP_plus_uconst.
 * plus_uconst is emitted for a few common FPU sizes; extend if needed.
 */
.macro cfi_irq_unwind_thread_frame
#if CFI_IRQ_CFA_OFFSET == 32
	.cfi_escape 0x0f, 5, 0x7d, 0x00, 0x06, 0x23, 0x20
#elif CFI_IRQ_CFA_OFFSET == 72
	.cfi_escape 0x0f, 5, 0x7d, 0x00, 0x06, 0x23, 0x48
#elif CFI_IRQ_CFA_OFFSET == 104
	.cfi_escape 0x0f, 5, 0x7d, 0x00, 0x06, 0x23, 0x68
#elif CFI_IRQ_CFA_OFFSET == 232
	.cfi_escape 0x0f, 6, 0x7d, 0x00, 0x06, 0x23, 0xe8, 0x01
#else
	/* Build-time: add DW_CFA_def_cfa_expression for this CFI_IRQ_CFA_OFFSET */
	.error "unsupported CFI_IRQ_CFA_OFFSET for IRQ unwind"
#endif
	cfi_basic_sf_offsets 0
	/* DW_CFA_val_offset: interrupted SP (system mode) = CFA + 0 */
	.cfi_escape 0x14, 0x0d, 0x00
.endm

.macro z_arm_cortex_ar_enter_exc
	/*
	 * Store r0-r3, r12, lr into the stack to construct an exception
	 * stack frame.
	 */
	srsdb sp!, #MODE_SYS
	cps #MODE_SYS
	push {r0-r3, r12, lr}

	/* TODO: EXTRA_EXCEPTION_INFO */
	mov r0, sp

	/* increment exception depth */
	get_cpu r2
	ldrb r1, [r2, #_cpu_offset_to_exc_depth]
	add r1, r1, #1
	strb r1, [r2, #_cpu_offset_to_exc_depth]
.endm

#endif /* _MACRO_PRIV_INC_ */
