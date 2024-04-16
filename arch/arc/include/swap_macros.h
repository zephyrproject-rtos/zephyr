/* swap_macros.h - helper macros for context switch */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_ARCH_ARC_INCLUDE_SWAP_MACROS_H_
#define ZEPHYR_ARCH_ARC_INCLUDE_SWAP_MACROS_H_

#include <zephyr/kernel_structs.h>
#include <offsets_short.h>
#include <zephyr/toolchain.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/arc/tool-compat.h>
#include <zephyr/arch/arc/asm-compat/assembler.h>
#include <zephyr/kernel.h>
#include "../core/dsp/swap_dsp_macros.h"

#ifdef _ASMLANGUAGE

/* save callee regs of current thread in r2 */
.macro _save_callee_saved_regs

	SUBR sp, sp, ___callee_saved_stack_t_SIZEOF

	/* save regs on stack */
	STR r13, sp, ___callee_saved_stack_t_r13_OFFSET
	STR r14, sp, ___callee_saved_stack_t_r14_OFFSET
	STR r15, sp, ___callee_saved_stack_t_r15_OFFSET
	STR r16, sp, ___callee_saved_stack_t_r16_OFFSET
	STR r17, sp, ___callee_saved_stack_t_r17_OFFSET
	STR r18, sp, ___callee_saved_stack_t_r18_OFFSET
	STR r19, sp, ___callee_saved_stack_t_r19_OFFSET
	STR r20, sp, ___callee_saved_stack_t_r20_OFFSET
	STR r21, sp, ___callee_saved_stack_t_r21_OFFSET
	STR r22, sp, ___callee_saved_stack_t_r22_OFFSET
	STR r23, sp, ___callee_saved_stack_t_r23_OFFSET
	STR r24, sp, ___callee_saved_stack_t_r24_OFFSET
	STR r25, sp, ___callee_saved_stack_t_r25_OFFSET
	STR r26, sp, ___callee_saved_stack_t_r26_OFFSET
	STR fp,  sp, ___callee_saved_stack_t_fp_OFFSET

#ifdef CONFIG_USERSPACE
#ifdef CONFIG_ARC_HAS_SECURE
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	lr r13, [_ARC_V2_SEC_U_SP]
	st_s r13, [sp, ___callee_saved_stack_t_user_sp_OFFSET]
	lr r13, [_ARC_V2_SEC_K_SP]
	st_s r13, [sp, ___callee_saved_stack_t_kernel_sp_OFFSET]
#else
	lr r13, [_ARC_V2_USER_SP]
	st_s r13, [sp, ___callee_saved_stack_t_user_sp_OFFSET]
	lr r13, [_ARC_V2_KERNEL_SP]
	st_s r13, [sp, ___callee_saved_stack_t_kernel_sp_OFFSET]
#endif /* CONFIG_ARC_SECURE_FIRMWARE */
#else
	lr r13, [_ARC_V2_USER_SP]
	st_s r13, [sp, ___callee_saved_stack_t_user_sp_OFFSET]
#endif
#endif
	STR r30, sp, ___callee_saved_stack_t_r30_OFFSET

#ifdef CONFIG_ARC_HAS_ACCL_REGS
	STR r58, sp, ___callee_saved_stack_t_r58_OFFSET
#ifndef CONFIG_64BIT
	STR r59, sp, ___callee_saved_stack_t_r59_OFFSET
#endif /* !CONFIG_64BIT */
#endif

#ifdef CONFIG_FPU_SHARING
	ld_s r13, [r2, ___thread_base_t_user_options_OFFSET]
	bbit0 r13, K_FP_IDX, fpu_skip_save
	lr r13, [_ARC_V2_FPU_STATUS]
	st_s r13, [sp, ___callee_saved_stack_t_fpu_status_OFFSET]
	lr r13, [_ARC_V2_FPU_CTRL]
	st_s r13, [sp, ___callee_saved_stack_t_fpu_ctrl_OFFSET]

#ifdef CONFIG_FP_FPU_DA
	lr r13, [_ARC_V2_FPU_DPFP1L]
	st_s r13, [sp, ___callee_saved_stack_t_dpfp1l_OFFSET]
	lr r13, [_ARC_V2_FPU_DPFP1H]
	st_s r13, [sp, ___callee_saved_stack_t_dpfp1h_OFFSET]
	lr r13, [_ARC_V2_FPU_DPFP2L]
	st_s r13, [sp, ___callee_saved_stack_t_dpfp2l_OFFSET]
	lr r13, [_ARC_V2_FPU_DPFP2H]
	st_s r13, [sp, ___callee_saved_stack_t_dpfp2h_OFFSET]
#endif
#endif
fpu_skip_save :
	_save_dsp_regs
	/* save stack pointer in struct k_thread */
	STR sp, r2, _thread_offset_to_sp
.endm

/* load the callee regs of thread (in r2)*/
.macro _load_callee_saved_regs
	/* restore stack pointer from struct k_thread */
	LDR sp, r2, _thread_offset_to_sp

#ifdef CONFIG_ARC_HAS_ACCL_REGS
	LDR r58, sp, ___callee_saved_stack_t_r58_OFFSET
#ifndef CONFIG_64BIT
	LDR r59, sp, ___callee_saved_stack_t_r59_OFFSET
#endif /* !CONFIG_64BIT */
#endif

#ifdef CONFIG_FPU_SHARING
	ld_s r13, [r2, ___thread_base_t_user_options_OFFSET]
	bbit0 r13, K_FP_IDX, fpu_skip_load

	ld_s r13, [sp, ___callee_saved_stack_t_fpu_status_OFFSET]
	sr r13, [_ARC_V2_FPU_STATUS]
	ld_s r13, [sp, ___callee_saved_stack_t_fpu_ctrl_OFFSET]
	sr r13, [_ARC_V2_FPU_CTRL]

#ifdef CONFIG_FP_FPU_DA
	ld_s r13, [sp, ___callee_saved_stack_t_dpfp1l_OFFSET]
	sr r13, [_ARC_V2_FPU_DPFP1L]
	ld_s r13, [sp, ___callee_saved_stack_t_dpfp1h_OFFSET]
	sr r13, [_ARC_V2_FPU_DPFP1H]
	ld_s r13, [sp, ___callee_saved_stack_t_dpfp2l_OFFSET]
	sr r13, [_ARC_V2_FPU_DPFP2L]
	ld_s r13, [sp, ___callee_saved_stack_t_dpfp2h_OFFSET]
	sr r13, [_ARC_V2_FPU_DPFP2H]
#endif
#endif
fpu_skip_load :
	_load_dsp_regs
#ifdef CONFIG_USERSPACE
#ifdef CONFIG_ARC_HAS_SECURE
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	ld_s r13, [sp, ___callee_saved_stack_t_user_sp_OFFSET]
	sr r13, [_ARC_V2_SEC_U_SP]
	ld_s r13, [sp, ___callee_saved_stack_t_kernel_sp_OFFSET]
	sr r13, [_ARC_V2_SEC_K_SP]
#else
	ld_s r13, [sp, ___callee_saved_stack_t_user_sp_OFFSET]
	sr r13, [_ARC_V2_USER_SP]
	ld_s r13, [sp, ___callee_saved_stack_t_kernel_sp_OFFSET]
	sr r13, [_ARC_V2_KERNEL_SP]
#endif /* CONFIG_ARC_SECURE_FIRMWARE */
#else
	ld_s r13, [sp, ___callee_saved_stack_t_user_sp_OFFSET]
	sr r13, [_ARC_V2_USER_SP]
#endif
#endif

	LDR r13, sp, ___callee_saved_stack_t_r13_OFFSET
	LDR r14, sp, ___callee_saved_stack_t_r14_OFFSET
	LDR r15, sp, ___callee_saved_stack_t_r15_OFFSET
	LDR r16, sp, ___callee_saved_stack_t_r16_OFFSET
	LDR r17, sp, ___callee_saved_stack_t_r17_OFFSET
	LDR r18, sp, ___callee_saved_stack_t_r18_OFFSET
	LDR r19, sp, ___callee_saved_stack_t_r19_OFFSET
	LDR r20, sp, ___callee_saved_stack_t_r20_OFFSET
	LDR r21, sp, ___callee_saved_stack_t_r21_OFFSET
	LDR r22, sp, ___callee_saved_stack_t_r22_OFFSET
	LDR r23, sp, ___callee_saved_stack_t_r23_OFFSET
	LDR r24, sp, ___callee_saved_stack_t_r24_OFFSET
	LDR r25, sp, ___callee_saved_stack_t_r25_OFFSET
	LDR r26, sp, ___callee_saved_stack_t_r26_OFFSET
	LDR fp,  sp, ___callee_saved_stack_t_fp_OFFSET
	LDR r30, sp, ___callee_saved_stack_t_r30_OFFSET

	ADDR sp, sp, ___callee_saved_stack_t_SIZEOF

.endm

/* discard callee regs */
.macro _discard_callee_saved_regs
	ADDR sp, sp, ___callee_saved_stack_t_SIZEOF
.endm

/*
 * Must be called with interrupts locked or in P0.
 * Upon exit, sp will be pointing to the stack frame.
 */
.macro _create_irq_stack_frame

	SUBR sp, sp, ___isf_t_SIZEOF

	STR blink, sp, ___isf_t_blink_OFFSET

	/* store these right away so we can use them if needed */

	STR r13, sp, ___isf_t_r13_OFFSET
	STR r12, sp, ___isf_t_r12_OFFSET
	STR r11, sp, ___isf_t_r11_OFFSET
	STR r10, sp, ___isf_t_r10_OFFSET
	STR r9,  sp, ___isf_t_r9_OFFSET
	STR r8,  sp, ___isf_t_r8_OFFSET
	STR r7,  sp, ___isf_t_r7_OFFSET
	STR r6,  sp, ___isf_t_r6_OFFSET
	STR r5,  sp, ___isf_t_r5_OFFSET
	STR r4,  sp, ___isf_t_r4_OFFSET
	STR r3,  sp, ___isf_t_r3_OFFSET
	STR r2,  sp, ___isf_t_r2_OFFSET
	STR r1,  sp, ___isf_t_r1_OFFSET
	STR r0,  sp, ___isf_t_r0_OFFSET

#ifdef CONFIG_ARC_HAS_ZOL
	MOVR r0, lp_count
	STR r0, sp, ___isf_t_lp_count_OFFSET
	LRR r1, [_ARC_V2_LP_START]
	LRR r0, [_ARC_V2_LP_END]
	STR r1, sp, ___isf_t_lp_start_OFFSET
	STR r0, sp, ___isf_t_lp_end_OFFSET
#endif /* CONFIG_ARC_HAS_ZOL */

#ifdef CONFIG_CODE_DENSITY
	lr r1, [_ARC_V2_JLI_BASE]
	lr r0, [_ARC_V2_LDI_BASE]
	lr r2, [_ARC_V2_EI_BASE]
	st_s r1, [sp, ___isf_t_jli_base_OFFSET]
	st_s r0, [sp, ___isf_t_ldi_base_OFFSET]
	st_s r2, [sp, ___isf_t_ei_base_OFFSET]
#endif

.endm

/*
 * Must be called with interrupts locked or in P0.
 * sp must be pointing the to stack frame.
 */
.macro _pop_irq_stack_frame

	LDR blink, sp, ___isf_t_blink_OFFSET

#ifdef CONFIG_CODE_DENSITY
	ld_s r1, [sp, ___isf_t_jli_base_OFFSET]
	ld_s r0, [sp, ___isf_t_ldi_base_OFFSET]
	ld_s r2, [sp, ___isf_t_ei_base_OFFSET]
	sr r1, [_ARC_V2_JLI_BASE]
	sr r0, [_ARC_V2_LDI_BASE]
	sr r2, [_ARC_V2_EI_BASE]
#endif

#ifdef CONFIG_ARC_HAS_ZOL
	LDR r0, sp, ___isf_t_lp_count_OFFSET
	MOVR lp_count, r0
	LDR r1, sp, ___isf_t_lp_start_OFFSET
	LDR r0, sp, ___isf_t_lp_end_OFFSET
	SRR r1, [_ARC_V2_LP_START]
	SRR r0, [_ARC_V2_LP_END]
#endif /* CONFIG_ARC_HAS_ZOL */

	LDR r13, sp, ___isf_t_r13_OFFSET
	LDR r12, sp, ___isf_t_r12_OFFSET
	LDR r11, sp, ___isf_t_r11_OFFSET
	LDR r10, sp, ___isf_t_r10_OFFSET
	LDR r9,  sp, ___isf_t_r9_OFFSET
	LDR r8,  sp, ___isf_t_r8_OFFSET
	LDR r7,  sp, ___isf_t_r7_OFFSET
	LDR r6,  sp, ___isf_t_r6_OFFSET
	LDR r5,  sp, ___isf_t_r5_OFFSET
	LDR r4,  sp, ___isf_t_r4_OFFSET
	LDR r3,  sp, ___isf_t_r3_OFFSET
	LDR r2,  sp, ___isf_t_r2_OFFSET
	LDR r1,  sp, ___isf_t_r1_OFFSET
	LDR r0,  sp, ___isf_t_r0_OFFSET


	/*
	 * All gprs have been reloaded, the only one that is still usable is
	 * ilink.
	 *
	 * The pc and status32 values will still be on the stack. We cannot
	 * pop them yet because the callers of _pop_irq_stack_frame must reload
	 * status32 differently depending on the execution context they are
	 * running in (arch_switch(), firq or exception).
	 */
	ADDR sp, sp, ___isf_t_SIZEOF

.endm

/*
 * To use this macro, r2 should have the value of thread struct pointer to
 * _kernel.current. r3 is a scratch reg.
 */
.macro _load_stack_check_regs
#if defined(CONFIG_ARC_SECURE_FIRMWARE)
	ld r3, [r2, _thread_offset_to_k_stack_base]
	sr r3, [_ARC_V2_S_KSTACK_BASE]
	ld r3, [r2, _thread_offset_to_k_stack_top]
	sr r3, [_ARC_V2_S_KSTACK_TOP]
#ifdef CONFIG_USERSPACE
	ld r3, [r2, _thread_offset_to_u_stack_base]
	sr r3, [_ARC_V2_S_USTACK_BASE]
	ld r3, [r2, _thread_offset_to_u_stack_top]
	sr r3, [_ARC_V2_S_USTACK_TOP]
#endif
#else /* CONFIG_ARC_HAS_SECURE */
	ld r3, [r2, _thread_offset_to_k_stack_base]
	sr r3, [_ARC_V2_KSTACK_BASE]
	ld r3, [r2, _thread_offset_to_k_stack_top]
	sr r3, [_ARC_V2_KSTACK_TOP]
#ifdef CONFIG_USERSPACE
	ld r3, [r2, _thread_offset_to_u_stack_base]
	sr r3, [_ARC_V2_USTACK_BASE]
	ld r3, [r2, _thread_offset_to_u_stack_top]
	sr r3, [_ARC_V2_USTACK_TOP]
#endif
#endif /* CONFIG_ARC_SECURE_FIRMWARE */
.endm

/* check and increase the interrupt nest counter
 * after increase, check whether nest counter == 1
 * the result will be EQ bit of status32
 * two temp regs are needed
 */
.macro _check_and_inc_int_nest_counter, reg1, reg2
#ifdef CONFIG_SMP
	/* get pointer to _cpu_t of this CPU */
	_get_cpu_id MACRO_ARG(reg1)
	ASLR MACRO_ARG(reg1), MACRO_ARG(reg1), ARC_REGSHIFT
	LDR MACRO_ARG(reg1), MACRO_ARG(reg1), _curr_cpu
	/* _cpu_t.nested is 32 bit despite of platform bittnes */
	ld MACRO_ARG(reg2), [MACRO_ARG(reg1), ___cpu_t_nested_OFFSET]
#else
	MOVR MACRO_ARG(reg1), _kernel
	/* z_kernel.nested is 32 bit despite of platform bittnes */
	ld MACRO_ARG(reg2), [MACRO_ARG(reg1), _kernel_offset_to_nested]
#endif
	add MACRO_ARG(reg2), MACRO_ARG(reg2), 1
#ifdef CONFIG_SMP
	st MACRO_ARG(reg2), [MACRO_ARG(reg1), ___cpu_t_nested_OFFSET]
#else
	st MACRO_ARG(reg2), [MACRO_ARG(reg1), _kernel_offset_to_nested]
#endif
	cmp MACRO_ARG(reg2), 1
.endm

/* decrease interrupt stack nest counter
 * the counter > 0, interrupt stack is used, or
 * not used
 */
.macro _dec_int_nest_counter, reg1, reg2
#ifdef CONFIG_SMP
	/* get pointer to _cpu_t of this CPU */
	_get_cpu_id MACRO_ARG(reg1)
	ASLR MACRO_ARG(reg1), MACRO_ARG(reg1), ARC_REGSHIFT
	LDR MACRO_ARG(reg1), MACRO_ARG(reg1), _curr_cpu
	/* _cpu_t.nested is 32 bit despite of platform bittnes */
	ld MACRO_ARG(reg2), [MACRO_ARG(reg1), ___cpu_t_nested_OFFSET]
#else
	MOVR MACRO_ARG(reg1), _kernel
	/* z_kernel.nested is 32 bit despite of platform bittnes */
	ld MACRO_ARG(reg2), [MACRO_ARG(reg1), _kernel_offset_to_nested]
#endif
	sub MACRO_ARG(reg2), MACRO_ARG(reg2), 1
#ifdef CONFIG_SMP
	st MACRO_ARG(reg2), [MACRO_ARG(reg1), ___cpu_t_nested_OFFSET]
#else
	st MACRO_ARG(reg2), [MACRO_ARG(reg1), _kernel_offset_to_nested]
#endif
.endm

/* If multi bits in IRQ_ACT are set, i.e. last bit != fist bit, it's
 * in nest interrupt. The result will be EQ bit of status32
 * need two temp reg to do this
 */
.macro _check_nest_int_by_irq_act, reg1, reg2
	lr MACRO_ARG(reg1), [_ARC_V2_AUX_IRQ_ACT]
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	and MACRO_ARG(reg1), MACRO_ARG(reg1), ((1 << ARC_N_IRQ_START_LEVEL) - 1)
#else
	and MACRO_ARG(reg1), MACRO_ARG(reg1), 0xffff
#endif
	ffs MACRO_ARG(reg2), MACRO_ARG(reg1)
	fls MACRO_ARG(reg1), MACRO_ARG(reg1)
	cmp MACRO_ARG(reg1), MACRO_ARG(reg2)
.endm


/* macro to get id of current cpu
 * the result will be in reg (a reg)
 */
.macro _get_cpu_id, reg
	LRR MACRO_ARG(reg), [_ARC_V2_IDENTITY]
	xbfu MACRO_ARG(reg), MACRO_ARG(reg), 0xe8
.endm

/* macro to get the interrupt stack of current cpu
 * the result will be in irq_sp (a reg)
 */
.macro _get_curr_cpu_irq_stack, irq_sp
#ifdef CONFIG_SMP
	/* get pointer to _cpu_t of this CPU */
	_get_cpu_id MACRO_ARG(irq_sp)
	ASLR MACRO_ARG(irq_sp), MACRO_ARG(irq_sp), ARC_REGSHIFT
	LDR MACRO_ARG(irq_sp), MACRO_ARG(irq_sp), _curr_cpu
	/* get pointer to irq_stack itself */
	LDR MACRO_ARG(irq_sp), MACRO_ARG(irq_sp), ___cpu_t_irq_stack_OFFSET
#else
	MOVR MACRO_ARG(irq_sp), _kernel
	LDR MACRO_ARG(irq_sp), MACRO_ARG(irq_sp), _kernel_offset_to_irq_stack
#endif
.endm

/* macro to push aux reg through reg */
.macro PUSHAX, reg, aux
	LRR MACRO_ARG(reg), [MACRO_ARG(aux)]
	PUSHR MACRO_ARG(reg)
.endm

/* macro to pop aux reg through reg */
.macro POPAX, reg, aux
	POPR MACRO_ARG(reg)
	SRR MACRO_ARG(reg), [MACRO_ARG(aux)]
.endm


/* macro to store old thread call regs */
.macro _store_old_thread_callee_regs

	_save_callee_saved_regs
	/* Save old thread into switch handle which is required by z_sched_switch_spin.
	 * NOTE: we shouldn't save anything related to old thread context after this point!
	 * TODO: we should add SMP write-after-write data memory barrier here, as we want all
	 * previous writes completed before setting switch_handle which is polled by other cores
	 * in z_sched_switch_spin in case of SMP. Though it's not likely that this issue
	 * will reproduce in real world as there is some gap before reading switch_handle and
	 * reading rest of the data we've stored before.
	 */
	STR r2, r2, ___thread_t_switch_handle_OFFSET
.endm

/* macro to store old thread call regs  in interrupt*/
.macro _irq_store_old_thread_callee_regs
#if defined(CONFIG_USERSPACE)
/*
 * when USERSPACE is enabled, according to ARCv2 ISA, SP will be switched
 * if interrupt comes out in user mode, and will be recorded in bit 31
 * (U bit) of IRQ_ACT. when interrupt exits, SP will be switched back
 * according to U bit.
 *
 * need to remember the user/kernel status of interrupted thread, will be
 * restored when thread switched back
 *
 */
	lr r1, [_ARC_V2_AUX_IRQ_ACT]
	and r3, r1, 0x80000000
	push_s r3

	bclr r1, r1, 31
	sr r1, [_ARC_V2_AUX_IRQ_ACT]
#endif
	_store_old_thread_callee_regs
.endm

/* macro to load new thread callee regs */
.macro _load_new_thread_callee_regs
#ifdef CONFIG_ARC_STACK_CHECKING
	_load_stack_check_regs
#endif
	/*
	 * _load_callee_saved_regs expects incoming thread in r2.
	 * _load_callee_saved_regs restores the stack pointer.
	 */
	_load_callee_saved_regs

#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
	push_s r2
	bl configure_mpu_thread
	pop_s r2
#endif
	/* _thread_arch.relinquish_cause is 32 bit despite of platform bittnes */
	ld r3, [r2, _thread_offset_to_relinquish_cause]
.endm


/* when switch to thread caused by coop, some status regs need to set */
.macro _set_misc_regs_irq_switch_from_coop
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	/* must return to secure mode, so set IRM bit to 1 */
	lr r0, [_ARC_V2_SEC_STAT]
	bset r0, r0, _ARC_V2_SEC_STAT_IRM_BIT
	sflag r0
#endif
.endm

/* when switch to thread caused by irq, some status regs need to set */
.macro _set_misc_regs_irq_switch_from_irq
#if defined(CONFIG_USERSPACE)
/*
 * need to recover the user/kernel status of interrupted thread
 */
	pop_s r3
	lr r2, [_ARC_V2_AUX_IRQ_ACT]
	or r2, r2, r3
	sr r2, [_ARC_V2_AUX_IRQ_ACT]
#endif

#ifdef CONFIG_ARC_SECURE_FIRMWARE
	/* here need to recover SEC_STAT.IRM bit */
	pop_s r3
	sflag r3
#endif
.endm

/* macro to get next switch handle in assembly */
.macro _get_next_switch_handle
	PUSHR r2
	MOVR r0, sp
	bl z_arch_get_next_switch_handle
	POPR  r2
.endm

/* macro to disable stack checking in assembly, need a GPR
 * to do this
 */
.macro _disable_stack_checking, reg
#ifdef CONFIG_ARC_STACK_CHECKING
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	lr MACRO_ARG(reg), [_ARC_V2_SEC_STAT]
	bclr MACRO_ARG(reg), MACRO_ARG(reg), _ARC_V2_SEC_STAT_SSC_BIT
	sflag MACRO_ARG(reg)

#else
	lr MACRO_ARG(reg), [_ARC_V2_STATUS32]
	bclr MACRO_ARG(reg), MACRO_ARG(reg), _ARC_V2_STATUS32_SC_BIT
	kflag MACRO_ARG(reg)
#endif
#endif
.endm

/* macro to enable stack checking in assembly, need a GPR
 * to do this
 */
.macro _enable_stack_checking, reg
#ifdef CONFIG_ARC_STACK_CHECKING
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	lr MACRO_ARG(reg), [_ARC_V2_SEC_STAT]
	bset MACRO_ARG(reg), MACRO_ARG(reg), _ARC_V2_SEC_STAT_SSC_BIT
	sflag MACRO_ARG(reg)
#else
	lr MACRO_ARG(reg), [_ARC_V2_STATUS32]
	bset MACRO_ARG(reg), MACRO_ARG(reg), _ARC_V2_STATUS32_SC_BIT
	kflag MACRO_ARG(reg)
#endif
#endif
.endm


#define __arc_u9_max		(255)
#define __arc_u9_min		(-256)
#define __arc_ldst32_as_shift	2

/*
 * When we accessing bloated struct member we can exceed u9 operand in store
 * instruction. So we can use _st32_huge_offset macro instead
 */
.macro _st32_huge_offset, d, s, offset, temp
	.if MACRO_ARG(offset) <= __arc_u9_max && MACRO_ARG(offset) >= __arc_u9_min
		st MACRO_ARG(d), [MACRO_ARG(s), MACRO_ARG(offset)]
	/* Technically we can optimize with .as both big positive and negative offsets here, but
	 * as we use only positive offsets in hand-written assembly code we keep only
	 * positive offset case here for simplicity.
	 */
	.elseif !(MACRO_ARG(offset) % (1 << __arc_ldst32_as_shift)) &&                             \
		MACRO_ARG(offset) <= (__arc_u9_max << __arc_ldst32_as_shift) &&                    \
		MACRO_ARG(offset) >= 0
		st.as MACRO_ARG(d), [MACRO_ARG(s), MACRO_ARG(offset) >> __arc_ldst32_as_shift]
	.else
		ADDR MACRO_ARG(temp), MACRO_ARG(s), MACRO_ARG(offset)
		st MACRO_ARG(d), [MACRO_ARG(temp)]
	.endif
.endm

#endif /* _ASMLANGUAGE */

#endif /*  ZEPHYR_ARCH_ARC_INCLUDE_SWAP_MACROS_H_ */
