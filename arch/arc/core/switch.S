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
#include <linker/sections.h>
#include <arch/cpu.h>
#include <v2/irq.h>
#include <swap_macros.h>

GTEXT(z_arc_switch)

/**
 *
 * @brief Initiate a cooperative context switch
 *
 * The arch_switch routine is invoked by various kernel services to effect
 * a cooperative context switch. Prior to invoking arch_switch, the caller
 * disables interrupts via irq_lock()

 * Given that arch_switch() is called to effect a cooperative context switch,
 * the caller-saved integer registers are saved on the stack by the function
 * call preamble to arch_switch. This creates a custom stack frame that will
 * be popped when returning from arch_switch, but is not suitable for handling
 * a return from an exception. Thus, the fact that the thread is pending because
 * of a cooperative call to arch_switch() has to be recorded via the
 * _CAUSE_COOP code in the relinquish_cause of the thread's k_thread structure.
 * The _rirq_exit()/_firq_exit() code will take care of doing the right thing
 * to restore the thread status.
 *
 * When arch_switch() is invoked, we know the decision to perform a context
 * switch or not has already been taken and a context switch must happen.
 *
 *
 * C function prototype:
 *
 * void arch_switch(void *switch_to, void **switched_from);
 *
 */

SECTION_FUNC(TEXT, z_arc_switch)

	/*
	 * r0 = new_thread->switch_handle = switch_to thread,
	 * r1 = &old_thread->switch_handle
	 * get old_thread from r1
	 */

	sub r2, r1, ___thread_t_switch_handle_OFFSET


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

	_store_old_thread_callee_regs

/* disable stack checking here, as sp will be changed to target
 * thread'sp
 */
	_disable_stack_checking r3

	mov_s r2, r0

	_load_new_thread_callee_regs

	breq r3, _CAUSE_RIRQ, _switch_return_from_rirq
	nop_s
	breq r3, _CAUSE_FIRQ, _switch_return_from_firq
	nop_s

	/* fall through to _switch_return_from_coop */

.align 4
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

#ifdef CONFIG_TRACING
	push_s blink

	bl sys_trace_thread_switched_in

	pop_s blink
#endif
	j_s [blink]


.align 4
_switch_return_from_rirq:
_switch_return_from_firq:

	_set_misc_regs_irq_switch_from_irq

	/* use lowest interrupt priority to simulate
	 * a interrupt return to load left regs of new
	 * thread
	 */

	lr r3, [_ARC_V2_AUX_IRQ_ACT]
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
#ifdef CONFIG_TRACING
	push_s blink

	bl sys_trace_thread_switched_in

	pop_s blink
#endif
	rtie
