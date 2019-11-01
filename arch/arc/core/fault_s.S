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
GTEXT(z_do_kernel_oops)
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
GTEXT(z_irq_do_offload);
#endif


/*
 * The exception handling will use top part of interrupt stack to
 * get smaller memory footprint, because exception is not frequent.
 * To reduce the impact on interrupt handling, especially nested interrupt
 * the top part of interrupt stack cannot be too large, so add a check
 * here
 */
#if CONFIG_ARC_EXCEPTION_STACK_SIZE > (CONFIG_ISR_STACK_SIZE >> 1)
#error "interrupt stack size is too small"
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
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_extension)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_div_zero)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_dc_error)
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_maligned)

_exc_entry:
	/*
	 * re-use the top part of interrupt stack as exception
	 * stack. If this top part is used by interrupt handling,
	 * and exception is raised, then here it's guaranteed that
	 * exception handling has necessary stack to use
	 */
	mov_s ilink, sp
	_get_curr_cpu_irq_stack sp
	sub sp, sp, (CONFIG_ISR_STACK_SIZE - CONFIG_ARC_EXCEPTION_STACK_SIZE)

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
	/* ERSEC_STAT is IOW/RAZ in normal mode */
	lr r0,[_ARC_V2_ERSEC_STAT]
	st_s r0, [sp, ___isf_t_sec_stat_OFFSET]
#endif
	lr r0,[_ARC_V2_ERSTATUS]
	st_s r0, [sp, ___isf_t_status32_OFFSET]
	lr r0,[_ARC_V2_ERET]
	st_s r0, [sp, ___isf_t_pc_OFFSET] /* eret into pc */

	/* sp is parameter of _Fault */
	mov_s r0, sp
	/* ilink is the thread's original sp */
	mov_s r1, ilink
	jl _Fault

_exc_return:
/* the exception cause must be fixed in exception handler when exception returns
 * directly, or exception will be repeated.
 *
 * If thread switch is raised in exception handler, the context of old thread will
 * not be saved, i.e., it cannot be recovered, because we don't know where the
 * exception comes out,  thread context?irq_context?nest irq context?
 */

#ifdef CONFIG_PREEMPT_ENABLED
#ifdef CONFIG_SMP
	bl z_arch_smp_switch_in_isr
	breq r0, 0, _exc_return_from_exc
	mov_s r2, r0
#else
	mov_s r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]

	/* check if the current thread needs to be rescheduled */
	ld_s r0, [r1, _kernel_offset_to_ready_q_cache]
	breq r0, r2, _exc_return_from_exc

	ld_s r2, [r1, _kernel_offset_to_ready_q_cache]
	st_s r2, [r1, _kernel_offset_to_current]
#endif

#ifdef CONFIG_ARC_SECURE_FIRMWARE
	/*
	 * sync up the ERSEC_STAT.ERM and SEC_STAT.IRM.
	 * use a fake interrupt return to simulate an exception turn.
	 * ERM and IRM record which mode the cpu should return, 1: secure
	 * 0: normal
	 */
	lr r3,[_ARC_V2_ERSEC_STAT]
	btst r3, 31
	bset.nz r3, r3, _ARC_V2_SEC_STAT_IRM_BIT
	bclr.z r3, r3, _ARC_V2_SEC_STAT_IRM_BIT
	sflag r3
#endif
	/* clear AE bit to forget this was an exception, and go to
	 * register bank0 (if exception is raised in firq with 2 reg
	 * banks, then we may be bank1)
	 */
#if defined(CONFIG_ARC_FIRQ) && CONFIG_RGF_NUM_BANKS != 1
	/* save r2 in ilink because of the possible following reg
	 * bank switch
	 */
	mov_s ilink, r2
#endif
	lr r3, [_ARC_V2_STATUS32]
	and r3,r3,(~(_ARC_V2_STATUS32_AE | _ARC_V2_STATUS32_RB(7)))
	kflag r3
	/* pretend lowest priority interrupt happened to use common handler
	 * if exception is raised in irq, i.e., _ARC_V2_AUX_IRQ_ACT !=0,
	 * ignore irq handling, we cannot return to irq handling which may
	 * raise exception again. The ignored interrupts will be re-triggered
	 * if not cleared, or re-triggered by interrupt sources, or just missed
	 */

#ifdef CONFIG_ARC_SECURE_FIRMWARE
	mov_s r3, (1 << (ARC_N_IRQ_START_LEVEL - 1))
#else
	mov_s r3, (1 << (CONFIG_NUM_IRQ_PRIO_LEVELS - 1))
#endif

#ifdef CONFIG_ARC_NORMAL_FIRMWARE
	push_s r2
	mov_s r0, _ARC_V2_AUX_IRQ_ACT
	mov_s r1, r3
	mov_s r6, ARC_S_CALL_AUX_WRITE
	sjli SJLI_CALL_ARC_SECURE
	pop_s r2
#else
	sr r3, [_ARC_V2_AUX_IRQ_ACT]
#endif

#if defined(CONFIG_ARC_FIRQ) && CONFIG_RGF_NUM_BANKS != 1
	mov r2, ilink
#endif

	/* Assumption: r2 has current thread */
	b _rirq_common_interrupt_swap
#endif

_exc_return_from_exc:
	ld_s r0, [sp, ___isf_t_pc_OFFSET]
	sr r0, [_ARC_V2_ERET]

	_pop_irq_stack_frame
	mov_s sp, ilink
	rtie


SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_trap)
	/* get the id of trap_s */
	lr ilink, [_ARC_V2_ECR]
	and ilink, ilink, 0x3f
#ifdef CONFIG_USERSPACE
	cmp ilink, _TRAP_S_CALL_SYSTEM_CALL
	bne _do_non_syscall_trap
	/* do sys_call */
	mov_s ilink, K_SYSCALL_LIMIT
	cmp r6, ilink
	blt valid_syscall_id

	mov_s r0, r6
	mov_s r6, K_SYSCALL_BAD

valid_syscall_id:
	/* create a sys call frame
	 * caller regs (r0 - 12) are saved in _create_irq_stack_frame
	 * ok to use them later
	 */
	_create_irq_stack_frame

#ifdef CONFIG_ARC_SECURE_FIRMWARE
	/* ERSEC_STAT is IOW/RAZ in normal mode */
	lr r0, [_ARC_V2_ERSEC_STAT]
	st_s r0, [sp, ___isf_t_sec_stat_OFFSET]
#endif
	lr r0,[_ARC_V2_ERET]
	st_s r0, [sp, ___isf_t_pc_OFFSET] /* eret into pc */
	lr r0,[_ARC_V2_ERSTATUS]
	st_s r0, [sp, ___isf_t_status32_OFFSET]

	bclr r0, r0, _ARC_V2_STATUS32_U_BIT
	sr r0, [_ARC_V2_ERSTATUS]

	mov_s r0, _arc_do_syscall
	sr r0, [_ARC_V2_ERET]

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


	/* check whether irq stack is used */
	_check_and_inc_int_nest_counter r0, r1

	bne.d exc_nest_handle
	mov_s r0, sp

	_get_curr_cpu_irq_stack sp
exc_nest_handle:
	push_s r0

	jl z_irq_do_offload

	pop sp

	_dec_int_nest_counter r0, r1

	lr  r0, [_ARC_V2_AUX_IRQ_ACT]
	and r0, r0, 0xffff
	cmp r0, 0
	bne _exc_return_from_exc

#ifdef CONFIG_PREEMPT_ENABLED
#ifdef CONFIG_SMP
	bl z_arch_smp_switch_in_isr
	breq r0, 0, _exc_return_from_irqoffload_trap
	mov_s r2, r1

	_save_callee_saved_regs

	st _CAUSE_RIRQ, [r2, _thread_offset_to_relinquish_cause]
	mov_s r2, r0
#else
	mov_s r1, _kernel
	ld_s r2, [r1, _kernel_offset_to_current]

	/* check if the current thread needs to be rescheduled */
	ld_s r0, [r1, _kernel_offset_to_ready_q_cache]
	breq r0, r2, _exc_return_from_irqoffload_trap
#endif
#ifdef CONFIG_ARC_SECURE_FIRMWARE
	/*
	 * sync up the ERSEC_STAT.ERM and SEC_STAT.IRM.
	 * use a fake interrupt return to simulate an exception turn.
	 * ERM and IRM record which mode the cpu should return, 1: secure
	 * 0: normal
	 */
	lr r3,[_ARC_V2_ERSEC_STAT]
	btst r3, 31
	bset.nz r3, r3, _ARC_V2_SEC_STAT_IRM_BIT
	bclr.z r3, r3, _ARC_V2_SEC_STAT_IRM_BIT
	sflag r3
	/* save  _ARC_V2_SEC_STAT */
	and r3, r3, 0xff
	push_s r3
#endif

	_save_callee_saved_regs

	st _CAUSE_RIRQ, [r2, _thread_offset_to_relinquish_cause]
	/* note: Ok to use _CAUSE_RIRQ since everything is saved */

	mov_s r2, r0
#ifndef CONFIG_SMP
	st_s r2, [r1, _kernel_offset_to_current]
#endif
	/* clear AE bit to forget this was an exception */
	lr r3, [_ARC_V2_STATUS32]
	and r3,r3,(~_ARC_V2_STATUS32_AE)
	kflag r3
	/* pretend lowest priority interrupt happened to use common handler */
	lr r3, [_ARC_V2_AUX_IRQ_ACT]

#ifdef CONFIG_ARC_SECURE_FIRMWARE
	or r3, r3, (1 << (ARC_N_IRQ_START_LEVEL - 1))
#else
	or r3, r3, (1 << (CONFIG_NUM_IRQ_PRIO_LEVELS - 1))
#endif

#ifdef CONFIG_ARC_NORMAL_FIRMWARE
	push_s r2
	mov_s r0, _ARC_V2_AUX_IRQ_ACT
	mov_s r1, r3
	mov_s r6, ARC_S_CALL_AUX_WRITE
	sjli SJLI_CALL_ARC_SECURE
	pop_s r2
#else
	sr r3, [_ARC_V2_AUX_IRQ_ACT]
#endif
	/* Assumption: r2 has current thread */
	b _rirq_common_interrupt_swap
#endif

_exc_return_from_irqoffload_trap:
	_pop_irq_stack_frame
	rtie
#endif /* CONFIG_IRQ_OFFLOAD */
	b _exc_entry
