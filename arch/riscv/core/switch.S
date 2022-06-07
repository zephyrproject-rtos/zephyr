/*
 * Copyright (c) 2022 BayLibre, SAS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include "asm_macros.inc"

/* Convenience macros for loading/storing register states. */

#if defined(CONFIG_RISCV_ISA_RV32E)
#define DO_CALLEE_SAVED(op, reg) \
	op ra, _thread_offset_to_ra(reg)	;\
	op tp, _thread_offset_to_tp(reg)	;\
	op s0, _thread_offset_to_s0(reg)	;\
	op s1, _thread_offset_to_s1(reg)
#else
#define DO_CALLEE_SAVED(op, reg) \
	op ra, _thread_offset_to_ra(reg)	;\
	op tp, _thread_offset_to_tp(reg)	;\
	op s0, _thread_offset_to_s0(reg)	;\
	op s1, _thread_offset_to_s1(reg)	;\
	op s2, _thread_offset_to_s2(reg)	;\
	op s3, _thread_offset_to_s3(reg)	;\
	op s4, _thread_offset_to_s4(reg)	;\
	op s5, _thread_offset_to_s5(reg)	;\
	op s6, _thread_offset_to_s6(reg)	;\
	op s7, _thread_offset_to_s7(reg)	;\
	op s8, _thread_offset_to_s8(reg)	;\
	op s9, _thread_offset_to_s9(reg)	;\
	op s10, _thread_offset_to_s10(reg)	;\
	op s11, _thread_offset_to_s11(reg)
#endif /* CONFIG_RISCV_ISA_RV32E */

#define DO_FP_CALLEE_SAVED(op, reg) \
	op fs0, _thread_offset_to_fs0(reg)	;\
	op fs1, _thread_offset_to_fs1(reg)	;\
	op fs2, _thread_offset_to_fs2(reg)	;\
	op fs3, _thread_offset_to_fs3(reg)	;\
	op fs4, _thread_offset_to_fs4(reg)	;\
	op fs5, _thread_offset_to_fs5(reg)	;\
	op fs6, _thread_offset_to_fs6(reg)	;\
	op fs7, _thread_offset_to_fs7(reg)	;\
	op fs8, _thread_offset_to_fs8(reg)	;\
	op fs9, _thread_offset_to_fs9(reg)	;\
	op fs10, _thread_offset_to_fs10(reg)	;\
	op fs11, _thread_offset_to_fs11(reg)

GTEXT(z_riscv_switch)
GTEXT(z_thread_mark_switched_in)
GTEXT(z_riscv_configure_stack_guard)

/* void z_riscv_switch(k_thread_t *switch_to, k_thread_t *switch_from) */
SECTION_FUNC(TEXT, z_riscv_switch)

	/* Save the old thread's callee-saved registers */
	DO_CALLEE_SAVED(sr, a1)

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/* Assess whether floating-point registers need to be saved. */
	lb t0, _thread_offset_to_user_options(a1)
	andi t0, t0, K_FP_REGS
	beqz t0, skip_store_fp_callee_saved

	frcsr t0
	sw t0, _thread_offset_to_fcsr(a1)
	DO_FP_CALLEE_SAVED(fsr, a1)
skip_store_fp_callee_saved:
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

	/* Save the old thread's stack pointer */
	sr sp, _thread_offset_to_sp(a1)

	/* Set thread->switch_handle = thread to mark completion */
	sr a1, ___thread_t_switch_handle_OFFSET(a1)

	/* Get the new thread's stack pointer */
	lr sp, _thread_offset_to_sp(a0)

#if defined(CONFIG_PMP_STACK_GUARD)
	/*
	 * Stack guard has priority over user space for PMP usage.
	 * Preserve a0 across following call. s0 is not yet restored.
	 */
	mv s0, a0
	call z_riscv_pmp_stackguard_enable
	mv a0, s0
#elif defined(CONFIG_USERSPACE)
	/*
	 * When stackguard is not enabled, we need to configure the PMP only
	 * at context switch time as the PMP is not in effect while inm-mode.
	 * (it is done on every exception return otherwise).
	 */
	lb t0, _thread_offset_to_user_options(a0)
	andi t0, t0, K_USER
	beqz t0, not_user_task
	mv s0, a0
	call z_riscv_pmp_usermode_enable
	mv a0, s0
not_user_task:
#endif

#if CONFIG_INSTRUMENT_THREAD_SWITCHING
	mv s0, a0
	call z_thread_mark_switched_in
	mv a0, s0
#endif

	/* Restore the new thread's callee-saved registers */
	DO_CALLEE_SAVED(lr, a0)

#if defined(CONFIG_FPU) && defined(CONFIG_FPU_SHARING)
	/* Determine if we need to restore floating-point registers. */
	lb t0, _thread_offset_to_user_options(a0)
	li t1, MSTATUS_FS_INIT
	andi t0, t0, K_FP_REGS
	beqz t0, no_fp

	/* Enable floating point access */
	csrs mstatus, t1

	/* Restore FP regs */
	lw t1, _thread_offset_to_fcsr(a0)
	fscsr t1
	DO_FP_CALLEE_SAVED(flr, a0)
	j 1f

no_fp:
	/* Disable floating point access */
	csrc mstatus, t1
1:
#endif /* CONFIG_FPU && CONFIG_FPU_SHARING */

	/* Return to arch_switch() or _irq_wrapper() */
	ret
