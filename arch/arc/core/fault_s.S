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
#include <sections.h>
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

	.balign 4
SECTION_VAR(BSS, saved_stack_pointer)
	.word 0

#if CONFIG_RGF_NUM_BANKS == 1
GDATA(_exception_stack)
SECTION_VAR(NOINIT, _exception_stack)
	.space 512
/* note: QUARK_SE_C1000_SS can't afford 512B */
#else
GDATA(_firq_stack)
#endif

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

	st sp, [saved_stack_pointer]
#if CONFIG_RGF_NUM_BANKS == 1
	mov_s sp, _exception_stack
	add sp, sp, 512
#else
	mov_s sp, _firq_stack
	add sp, sp, CONFIG_FIRQ_STACK_SIZE
#endif

	/* save caller saved registers */
	_create_irq_stack_frame

	lr r0,[_ARC_V2_ERSTATUS]
	st_s r0, [sp, ___isf_t_status32_OFFSET]
	lr r0,[_ARC_V2_ERET]
	st_s r0, [sp, ___isf_t_pc_OFFSET] /* eret into pc */

	jl _Fault

	/* if _Fault returns, restore the registers */
	_pop_irq_stack_frame

	/* now restore the stack */
	ld sp,[saved_stack_pointer]
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

	jl _irq_do_offload

	mov_s r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]
	/* check if we're a nested interrupt: if so, let the
	 * interrupted interrupt handle the reschedule
	 */
	lr r3, [_ARC_V2_AUX_IRQ_ACT]
	/* the OS on ARCv2 always runs in kernel mode, so assume bit31 [U] in
	 * AUX_IRQ_ACT is always 0: if the contents of AUX_IRQ_ACT is 0, it
	 * means trap was taken from outside an interrupt handler.
	 * But if it was inside, let that handler do the swap.
	 */
	breq r3, 0, _trap_check_for_swap
_trap_return:
	_pop_irq_stack_frame
	rtie

.balign 4
_trap_check_for_swap:
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
