/*
 * Copyright (c) 2019 Synopsys.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread context switching
 *
 * This module implements the routines necessary for thread context switching
 * on ARCv2 CPUs.
 *
 * See isr_wrapper.S for details.
 */

#include <kernel_structs.h>
#include <offsets_short.h>
#include <toolchain.h>
#include <arch/cpu.h>
#include <v2/irq.h>
#include <swap_macros.h>

GTEXT(z_arch_switch)

/**
 *
 * @brief Initiate a cooperative context switch
 *
 * The z_arch_switch routine is invoked by various kernel services to effect
 * a cooperative context switch. Prior to invoking z_arch_switch, the caller
 * disables interrupts via irq_lock()

 * Given that z_arch_switch() is called to effect a cooperative context switch,
 * the caller-saved integer registers are saved on the stack by the function
 * call preamble to z_arch_switch. This creates a custom stack frame that will
 * be popped when returning from z_arch_switch, but is not suitable for handling
 * a return from an exception. Thus, the fact that the thread is pending because
 * of a cooperative call to z_arch_switch() has to be recorded via the
 * _CAUSE_COOP code in the relinquish_cause of the thread's k_thread structure.
 * The _rirq_exit()/_firq_exit() code will take care of doing the right thing
 * to restore the thread status.
 *
 * When z_arch_switch() is invoked, we know the decision to perform a context
 * switch or not has already been taken and a context switch must happen.
 *
 *
 * C function prototype:
 *
 * void z_arch_switch(void *switch_to, void **switched_from);
 *
 */

SECTION_FUNC(TEXT, z_arch_switch)

#ifdef CONFIG_EXECUTION_BENCHMARKING
	push_s r0
	push_s r1
	push_s blink

	bl read_timer_start_of_swap

	pop_s blink
	pop_s r1
	pop_s r0
#endif
	/*
	 * r0 = new_thread->switch_handle = switch_to thread,
	 * r1 = &old_thread->switch_handle = &switch_from thread
	 */

	ld_s r2, [r1]
	/*
	 * r2 may be dummy_thread in z_cstart, dummy_thread->switch_handle
	 * must be 0
	 */
	breq r2, 0, _switch_to_target_thread

	st _CAUSE_COOP, [r2, _thread_offset_to_relinquish_cause]

	/*
	 * Save status32 and blink on the stack before the callee-saved registers.
	 * This is the same layout as the start of an IRQ stack frame.
	 */
	lr r3, [_ARC_V2_STATUS32]
	push_s r3

#ifdef CONFIG_ARC_HAS_SECURE
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	lr r3, [_ARC_V2_SEC_STAT]
#else
	mov_s r3, 0
#endif
	push_s r3
#endif

	push_s blink

	_save_callee_saved_regs

#ifdef CONFIG_ARC_STACK_CHECKING
/* disable stack checking here, as sp will be changed to target
 * thread'sp
 */
#if defined(CONFIG_ARC_HAS_SECURE) && defined(CONFIG_ARC_SECURE_FIRMWARE)
	bclr r3, r3, _ARC_V2_SEC_STAT_SSC_BIT
	sflag r3
#else
	bclr r3, r3, _ARC_V2_STATUS32_SC_BIT
	kflag r3
#endif
#endif

_switch_to_target_thread:

	mov_s r2, r0

	/* entering here, r2 contains the new current thread */
#ifdef CONFIG_ARC_STACK_CHECKING
	_load_stack_check_regs
#endif

	_load_callee_saved_regs

#if defined(CONFIG_MPU_STACK_GUARD) || defined(CONFIG_USERSPACE)
	push_s r2
	bl configure_mpu_thread
	pop_s r2
#endif

	ld r3, [r2, _thread_offset_to_relinquish_cause]

	breq r3, _CAUSE_RIRQ, _switch_return_from_rirq
	nop_s
	breq r3, _CAUSE_FIRQ, _switch_return_from_firq
	nop_s

	/* fall through to _switch_return_from_coop */

.balign 4
_switch_return_from_coop:

	pop_s blink /* pc into blink */
#ifdef CONFIG_ARC_HAS_SECURE
	pop_s r3    /* pop SEC_STAT */
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	sflag r3
#endif
#endif
	pop_s r3    /* status32 into r3 */
	kflag r3    /* write status32 */

#ifdef CONFIG_EXECUTION_BENCHMARKING
	b _capture_value_for_benchmarking
#endif
return_loc:
	j_s [blink]


.balign 4
_switch_return_from_rirq:
_switch_return_from_firq:

#ifdef CONFIG_ARC_SECURE_FIRMWARE
	/* here need to recover SEC_STAT.IRM bit */
	pop_s r3
	sflag r3
#endif

	lr r3, [_ARC_V2_AUX_IRQ_ACT]
	/* use lowest interrupt priority */
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	or r3, r3, (1 << (ARC_N_IRQ_START_LEVEL - 1))
#else
	or r3, r3, (1 << (CONFIG_NUM_IRQ_PRIO_LEVELS - 1))
#endif

#ifdef CONFIG_ARC_NORMAL_FIRMWARE
	mov_s r0, _ARC_V2_AUX_IRQ_ACT
	mov_s r1, r3
	mov_s r6, ARC_S_CALL_AUX_WRITE
	sjli SJLI_CALL_ARC_SECURE
#else
	sr r3, [_ARC_V2_AUX_IRQ_ACT]
#endif
	rtie

#ifdef CONFIG_EXECUTION_BENCHMARKING
	.balign 4
_capture_value_for_benchmarking:
	push_s blink

	bl read_timer_end_of_swap

	pop_s blink
	b return_loc
#endif /* CONFIG_EXECUTION_BENCHMARKING */
