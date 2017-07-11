/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Fault handlers for ARCv2
 *
 * Fault handlers for ARCv2 processors.
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <arch/cpu.h>
#include <swap_macros.h>

GTEXT(_Fault)

GTEXT(__reset)
GTEXT(__memory_error)
GTEXT(__instruction_error)
GTEXT(__ev_machine_check)
GTEXT(__ev_tlb_miss_i)
GTEXT(__ev_tlb_miss_d)
GTEXT(__ev_prot_v)
GTEXT(__ev_privilege_v)
GTEXT(__ev_swi)
GTEXT(__ev_trap)
GTEXT(__ev_extension)
GTEXT(__ev_div_zero)
GTEXT(__ev_dc_error)
GTEXT(__ev_maligned)

GDATA(exc_nest_count)

/*
 * @brief Fault handler installed in the fault and reserved vectors
 */

SECTION_SUBSEC_FUNC(TEXT,__fault,__memory_error)
SECTION_SUBSEC_FUNC(TEXT,__fault,__instruction_error)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_machine_check)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_tlb_miss_i)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_tlb_miss_d)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_prot_v)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_privilege_v)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_swi)
#ifndef CONFIG_IRQ_OFFLOAD
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_trap)
#endif
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_extension)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_div_zero)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_dc_error)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_maligned)

	/*
	 * Before invoking exception handler, the kernel switches to an exception
	 * stack, to save the faulting thread's registers.
	 * The exception is fatal and all the kernel can do is just print
	 * a diagnostic message and halt.
	 */

#ifdef CONFIG_ARC_STACK_CHECKING
	push_s r2
	/* disable stack checking */
	lr r2, [_ARC_V2_STATUS32]
	bclr r2, r2, _ARC_V2_STATUS32_SC_BIT
	kflag r2
	pop_s r2
#endif

	/* save caller saved registers */
	_create_irq_stack_frame

	lr r0,[_ARC_V2_ERSTATUS]
	st_s r0, [sp, ___isf_t_status32_OFFSET]
	lr r0,[_ARC_V2_ERET]
	st_s r0, [sp, ___isf_t_pc_OFFSET] /* eret into pc */


	ld r1, [exc_nest_count]
	add r0, r1, 1
	st r0, [exc_nest_count]
	cmp r1, 0

	bgt.d  exc_nest_handle
	mov r0, sp

	mov r1, _kernel
	ld sp, [r1, _kernel_offset_to_irq_stack]
exc_nest_handle:
	push_s r0

	jl _Fault

	pop sp

	mov	r1, exc_nest_count
	ld	r0, [r1]
	sub	r0, r0, 1
	st 	r0, [r1]

	/* if _Fault returns, restore the registers */
	_pop_irq_stack_frame

	rtie

#ifdef CONFIG_IRQ_OFFLOAD
GTEXT(_irq_do_offload);

SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_trap)
	/*
	 * Before invoking exception handler, the kernel switches to an exception
	 * stack to save the faulting thread's registers.
	 * The exception is fatal and all the kernel can do is just print
	 * a diagnostic message and halt.
	 */

#ifdef CONFIG_ARC_STACK_CHECKING
	push_s r2
	/* disable stack checking */
	lr r2, [_ARC_V2_STATUS32]
	bclr r2, r2, _ARC_V2_STATUS32_SC_BIT
	kflag r2
	pop_s r2
#endif

	/* save caller saved registers */
	_create_irq_stack_frame

	lr r0,[_ARC_V2_ERSTATUS]
	st_s r0, [sp, ___isf_t_status32_OFFSET]
	lr r0,[_ARC_V2_ERET]
	st_s r0, [sp, ___isf_t_pc_OFFSET] /* eret into pc */

	ld r1, [exc_nest_count]
	add r0, r1, 1
	st r0, [exc_nest_count]
	cmp r1, 0

	bgt.d  trap_nest_handle
	mov r0, sp

	mov r1, _kernel
	ld sp, [r1, _kernel_offset_to_irq_stack]
trap_nest_handle:
	push_s r0

	jl _irq_do_offload

	pop sp

	/* check if we're a nested interrupt: if so, let the
	 * interrupted interrupt handle the reschedule
	 */
	mov	r1, exc_nest_count
	ld	r0, [r1]
	sub	r0, r0, 1
	cmp	r0, 0
	beq.d	_trap_check_for_swap
	st 	r0, [r1]

_trap_return:
	_pop_irq_stack_frame
	rtie

.balign 4
_trap_check_for_swap:
	mov_s r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]
	/*
	 * Non-preemptible thread ? Do not schedule (see explanation of
	 * preempt field in kernel_struct.h).
	 */
	ldh_s r0, [r2, _thread_offset_to_preempt]
	brhs r0, _NON_PREEMPT_THRESHOLD, _trap_return

	/* check if the current thread needs to be rescheduled */
	ld_s r0, [r1, _kernel_offset_to_ready_q_cache]
	breq r0, r2, _trap_return

	_save_callee_saved_regs

	st _CAUSE_RIRQ, [r2, _thread_offset_to_relinquish_cause]
	/* note: Ok to use _CAUSE_RIRQ since everything is saved */

	ld_s r2, [r1, _kernel_offset_to_ready_q_cache]
	st_s r2, [r1, _kernel_offset_to_current]

	/* clear AE bit to forget this was an exception */
	lr r3, [_ARC_V2_STATUS32]
	and r3,r3,(~_ARC_V2_STATUS32_AE)
	kflag r3
	/* pretend lowest priority interrupt happened to use common handler */
	lr r3, [_ARC_V2_AUX_IRQ_ACT]
	or r3,r3,(1<<(CONFIG_NUM_IRQ_PRIO_LEVELS-1)) /* use lowest */
	sr r3, [_ARC_V2_AUX_IRQ_ACT]

	/* Assumption: r2 has current thread */
	b _rirq_common_interrupt_swap

#endif /* CONFIG_IRQ_OFFLOAD */
