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

#define DO_CALLEE_SAVED(op, reg) \
	RV_E(	op ra, _thread_offset_to_ra(reg)	);\
	RV_E(	op s0, _thread_offset_to_s0(reg)	);\
	RV_E(	op s1, _thread_offset_to_s1(reg)	);\
	RV_I(	op s2, _thread_offset_to_s2(reg)	);\
	RV_I(	op s3, _thread_offset_to_s3(reg)	);\
	RV_I(	op s4, _thread_offset_to_s4(reg)	);\
	RV_I(	op s5, _thread_offset_to_s5(reg)	);\
	RV_I(	op s6, _thread_offset_to_s6(reg)	);\
	RV_I(	op s7, _thread_offset_to_s7(reg)	);\
	RV_I(	op s8, _thread_offset_to_s8(reg)	);\
	RV_I(	op s9, _thread_offset_to_s9(reg)	);\
	RV_I(	op s10, _thread_offset_to_s10(reg)	);\
	RV_I(	op s11, _thread_offset_to_s11(reg)	)

GTEXT(z_riscv_switch)
GTEXT(z_thread_mark_switched_in)
GTEXT(z_riscv_configure_stack_guard)
GTEXT(z_riscv_fpu_thread_context_switch)

/* void z_riscv_switch(k_thread_t *switch_to, k_thread_t *switch_from) */
SECTION_FUNC(TEXT, z_riscv_switch)

	/* Save the old thread's callee-saved registers */
	DO_CALLEE_SAVED(sr, a1)

	/* Save the old thread's stack pointer */
	sr sp, _thread_offset_to_sp(a1)

	/* Set thread->switch_handle = thread to mark completion */
	sr a1, ___thread_t_switch_handle_OFFSET(a1)

	/* Get the new thread's stack pointer */
	lr sp, _thread_offset_to_sp(a0)

#if defined(CONFIG_THREAD_LOCAL_STORAGE)
	/* Get the new thread's tls pointer */
	lr tp, _thread_offset_to_tls(a0)
#endif

#if defined(CONFIG_FPU_SHARING)
	 /* Preserve a0 across following call. s0 is not yet restored. */
	mv s0, a0
	call z_riscv_fpu_thread_context_switch
	mv a0, s0
#endif

#if defined(CONFIG_PMP_STACK_GUARD)
	/* Stack guard has priority over user space for PMP usage. */
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

	/* Return to arch_switch() or _irq_wrapper() */
	ret
