/*
 * Copyright (c) 2014 Wind River Systems, Inc.
 * Copyright (c) 2018 Synopsys.
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
#include <syscall.h>

GTEXT(_Fault)
GTEXT(_do_kernel_oops)
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
#ifdef CONFIG_IRQ_OFFLOAD
GTEXT(_irq_do_offload);
#endif

GDATA(exc_nest_count)

	.balign 4
SECTION_VAR(BSS, saved_value)
	.word 0

/* the necessary stack size for exception handling */
#define EXCEPTION_STACK_SIZE 384

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
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_extension)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_div_zero)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_dc_error)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_maligned)

_exc_entry:
	st sp, [saved_value]
	/*
	 * re-use the top part of interrupt stack as exception
	 * stack. If this top part is used by interrupt handling,
	 * and exception is raised, then here it's guaranteed that
	 * exception handling has necessary stack to use
	 */
	mov_s sp, _interrupt_stack
	add sp, sp, EXCEPTION_STACK_SIZE

	/*
	 * save caller saved registers
	 * this stack frame is set up in exception stack,
	 * not in the original sp (thread stack or interrupt stack).
	 * Because the exception may be raised by stack checking or
	 * mpu protect violation related to stack. If this stack frame
	 * is setup in original sp, double exception may be raised during
	 * _create_irq_stack_frame, which is unrecoverable.
	 */
	_create_irq_stack_frame

#ifdef CONFIG_ARC_HAS_SECURE
	lr r0,[_ARC_V2_ERSEC_STAT]
	st_s r0, [sp, ___isf_t_sec_stat_OFFSET]
#endif
	lr r0,[_ARC_V2_ERSTATUS]
	st_s r0, [sp, ___isf_t_status32_OFFSET]
	lr r0,[_ARC_V2_ERET]
	st_s r0, [sp, ___isf_t_pc_OFFSET] /* eret into pc */

	/* sp is parameter of _Fault */
	mov r0, sp
	jl _Fault

_exc_return:

#ifdef CONFIG_PREEMPT_ENABLED
	mov_s r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]

	/* check if the current thread needs to be rescheduled */
	ld_s r0, [r1, _kernel_offset_to_ready_q_cache]
	breq r0, r2, _exc_return_from_exc

	ld_s r2, [r1, _kernel_offset_to_ready_q_cache]
	st_s r2, [r1, _kernel_offset_to_current]

#ifdef CONFIG_ARC_HAS_SECURE
	/*
	 * sync up the ERSEC_STAT.ERM and SEC_STAT.IRM.
	 * use a fake interrupt return to simulate an exception turn.
	 * ERM and IRM record which mode the cpu should return, 1: secure
	 * 0: normal
	 */
	lr r3,[_ARC_V2_ERSEC_STAT]
	btst r3, 31
	bset.nz r3, r3, 3
	bclr.z r3, r3, 3
	/* sflag r3 */
	/* sflag instruction is not supported in current ARC GNU */
	.long 0x00ff302f
#endif
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
#endif

_exc_return_from_exc:
	ld_s r0, [sp, ___isf_t_pc_OFFSET]
	sr r0, [_ARC_V2_ERET]

	_pop_irq_stack_frame
	ld sp, [saved_value]
	rtie


SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_trap)
	/* get the id of trap_s */
	lr ilink, [_ARC_V2_ECR]
	and ilink, ilink, 0x3f
#ifdef CONFIG_USERSPACE
	cmp ilink, _TRAP_S_CALL_SYSTEM_CALL
	bne _do_non_syscall_trap
/* do sys_call */
	mov ilink, K_SYSCALL_LIMIT
	cmp r6, ilink
	blt valid_syscall_id

	mov r0, r6
	mov r6, K_SYSCALL_BAD

valid_syscall_id:
#ifdef CONFIG_ARC_HAS_SECURE
	lr ilink, [_ARC_V2_ERSEC_STAT]
	push ilink
#endif
	lr ilink, [_ARC_V2_ERET]
	push ilink
	lr ilink, [_ARC_V2_ERSTATUS]
	push ilink


	bclr ilink, ilink, _ARC_V2_STATUS32_U_BIT
	sr ilink, [_ARC_V2_ERSTATUS]

	mov ilink, _arc_do_syscall
	sr ilink, [_ARC_V2_ERET]

	rtie

_do_non_syscall_trap:
#endif /* CONFIG_USERSPACE */
#ifdef CONFIG_IRQ_OFFLOAD
	/*
	 * IRQ_OFFLOAD is to simulate interrupt handling through exception,
	 * so its entry is different with normal exception handling, it is
	 * handled in isr stack
	 */
	cmp ilink, _TRAP_S_SCALL_IRQ_OFFLOAD
	bne _exc_entry
	/* save caller saved registers */
	_create_irq_stack_frame

#ifdef CONFIG_ARC_HAS_SECURE
	lr r0,[_ARC_V2_ERSEC_STAT]
	st_s r0, [sp, ___isf_t_sec_stat_OFFSET]
#endif
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

	jl _irq_do_offload

	pop sp

	mov	r1, exc_nest_count
	ld	r0, [r1]
	sub	r0, r0, 1
	cmp	r0, 0
	bne.d	_exc_return_from_exc
	st 	r0, [r1]

#ifdef CONFIG_PREEMPT_ENABLED
	mov_s r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]

	/* check if the current thread needs to be rescheduled */
	ld_s r0, [r1, _kernel_offset_to_ready_q_cache]
	breq r0, r2, _exc_return_from_irqoffload_trap

	_save_callee_saved_regs

	st _CAUSE_RIRQ, [r2, _thread_offset_to_relinquish_cause]
	/* note: Ok to use _CAUSE_RIRQ since everything is saved */

	ld_s r2, [r1, _kernel_offset_to_ready_q_cache]
	st_s r2, [r1, _kernel_offset_to_current]

#ifdef CONFIG_ARC_HAS_SECURE
	/*
	 * sync up the ERSEC_STAT.ERM and SEC_STAT.IRM.
	 * use a fake interrupt return to simulate an exception turn.
	 * ERM and IRM record which mode the cpu should return, 1: secure
	 * 0: normal
	 */
	lr r3,[_ARC_V2_ERSEC_STAT]
	btst r3, 31
	bset.nz r3, r3, 3
	bclr.z r3, r3, 3
	/* sflag r3 */
	/* sflag instruction is not supported in current ARC GNU */
	.long 0x00ff302f
#endif
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
#endif

_exc_return_from_irqoffload_trap:
	_pop_irq_stack_frame
	rtie
#endif /* CONFIG_IRQ_OFFLOAD */
	b _exc_entry
