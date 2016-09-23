/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 * @brief Fault handlers for ARCv2
 *
 * Fault handlers for ARCv2 processors.
 */

#define _ASMLANGUAGE

#include <toolchain.h>
#include <sections.h>
#include <arch/cpu.h>
#include "swap_macros.h"

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
GDATA(_firq_stack)

SECTION_VAR(BSS, saved_stack_pointer)
	.word 0

#if CONFIG_NUM_IRQ_PRIO_LEVELS == 1
#error "NUM_IRQ_PRIO_LEVELS==1 is not supported."
/* The code below sets bit 1 in AUX_IRQ_ACT and thus requires
 * priority 0 and 1 at a minimum. Supporting only 1 priority
 * requires a change to this file but also changes to make
 * FIRQ optional.
 */
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
	 * stack, which is really the FIRQ stack, to save the faulting thread's
	 * registers. It can use the FIRQ stack because it knows it is unused
	 * since it is save to assume that if an exception has happened in FIRQ
	 * handler, the problem is fatal and all the kernel can do is just print
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
	mov_s sp, _firq_stack
	add sp, sp, CONFIG_FIRQ_STACK_SIZE

	/* save caller saved registers */
	_create_irq_stack_frame

	lr r0,[_ARC_V2_ERSTATUS]
	st_s r0, [sp, __tISF_status32_OFFSET]
	lr r0,[_ARC_V2_ERET]
	st_s r0, [sp, __tISF_pc_OFFSET] /* eret into pc */

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
	 * stack, which is really the FIRQ stack, to save the faulting thread's
	 * registers. It can use the FIRQ stack because it knows it is unused
	 * since it is safe to assume that if an exception has happened in FIRQ
	 * handler, the problem is fatal and all the kernel can do is just print
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

#ifndef CONFIG_MICROKERNEL
	st sp, [saved_stack_pointer]
	mov_s sp, _firq_stack
	add sp, sp, CONFIG_FIRQ_STACK_SIZE
#endif

	/* save caller saved registers */
	_create_irq_stack_frame

	lr r0,[_ARC_V2_ERSTATUS]
	st_s r0, [sp, __tISF_status32_OFFSET]
	lr r0,[_ARC_V2_ERET]
	st_s r0, [sp, __tISF_pc_OFFSET] /* eret into pc */

	jl _irq_do_offload

#ifdef CONFIG_MICROKERNEL
	mov_s r1, _nanokernel
	ld_s r2, [r1, __tNANO_current_OFFSET]
#if CONFIG_NUM_IRQ_PRIO_LEVELS > 1
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
#endif

.balign 4
_trap_check_for_swap:
	ld_s r0, [r2, __tTCS_flags_OFFSET]
	and.f r0, r0, PREEMPTIBLE
	bnz _e_check_if_a_fiber_is_ready
	b _trap_return

.balign 4
_e_check_if_a_fiber_is_ready:
	ld_s r0, [r1, __tNANO_fiber_OFFSET] /* incoming fiber in r0 */
	brne r0, 0, _trap_reschedule
	b _trap_return

.balign 4
_trap_reschedule:

	_save_callee_saved_regs

	st _CAUSE_RIRQ, [r2, __tTCS_relinquish_cause_OFFSET]
	/* note: Ok to use _CAUSE_RIRQ since everything is saved */

	ld_s r2, [r1, __tNANO_fiber_OFFSET]

	st_s r2, [r1, __tNANO_current_OFFSET]
	ld_s r3, [r2, __tTCS_link_OFFSET]
	st_s r3, [r1, __tNANO_fiber_OFFSET]

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
#else
	/* Nanokernel-only just returns from exception */

	/* if _Fault returns, restore the registers */
	_pop_irq_stack_frame

	/* now restore the stack */
	ld sp,[saved_stack_pointer]
	rtie
#endif

#endif /* CONFIG_IRQ_OFFLOAD */
