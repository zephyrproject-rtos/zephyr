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

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <swap_macros.h>
#include <zephyr/syscall.h>
#include <zephyr/arch/arc/asm-compat/assembler.h>

GTEXT(z_arc_fault)
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

.macro _save_exc_regs_into_stack
#ifdef CONFIG_ARC_HAS_SECURE
	/* ERSEC_STAT is IOW/RAZ in normal mode */
	lr r0,[_ARC_V2_ERSEC_STAT]
	st_s r0, [sp, ___isf_t_sec_stat_OFFSET]
#endif
	LRR r0, [_ARC_V2_ERET]
	STR r0, sp, ___isf_t_pc_OFFSET
	LRR r0, [_ARC_V2_ERSTATUS]
	STR r0, sp, ___isf_t_status32_OFFSET
.endm

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
	MOVR ilink, sp
	_get_curr_cpu_irq_stack sp
	SUBR sp, sp, (CONFIG_ISR_STACK_SIZE - CONFIG_ARC_EXCEPTION_STACK_SIZE)

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

	_save_exc_regs_into_stack

	/* sp is parameter of z_arc_fault */
	MOVR r0, sp
	/* ilink is the thread's original sp */
	MOVR r1, ilink
	jl z_arc_fault

_exc_return:
/* the exception cause must be fixed in exception handler when exception returns
 * directly, or exception will be repeated.
 *
 * If thread switch is raised in exception handler, the context of old thread will
 * not be saved, i.e., it cannot be recovered, because we don't know where the
 * exception comes out,  thread context?irq_context?nest irq context?
 */

	_get_next_switch_handle

	BREQR r0, 0, _exc_return_from_exc

	/* Save old thread into switch handle which is required by z_sched_switch_spin which
	 * will be called during old thread abort.
	 */
	STR r2, r2, ___thread_t_switch_handle_OFFSET

	MOVR r2, r0

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
	mov ilink, r2
#endif
	LRR r3, [_ARC_V2_STATUS32]
	ANDR r3, r3, (~(_ARC_V2_STATUS32_AE | _ARC_V2_STATUS32_RB(7)))
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
	MOVR r3, (1 << (CONFIG_NUM_IRQ_PRIO_LEVELS - 1))
#endif

#ifdef CONFIG_ARC_NORMAL_FIRMWARE
	push_s r2
	mov_s r0, _ARC_V2_AUX_IRQ_ACT
	mov_s r1, r3
	mov_s r6, ARC_S_CALL_AUX_WRITE
	sjli SJLI_CALL_ARC_SECURE
	pop_s r2
#else
	SRR r3, [_ARC_V2_AUX_IRQ_ACT]
#endif

#if defined(CONFIG_ARC_FIRQ) && CONFIG_RGF_NUM_BANKS != 1
	mov r2, ilink
#endif

	/* Assumption: r2 has next thread */
	b _rirq_newthread_switch

_exc_return_from_exc:
	/* exception handler may change return address.
	 * reload it
	 */
	LDR r0, sp, ___isf_t_pc_OFFSET
	SRR r0, [_ARC_V2_ERET]

	_pop_irq_stack_frame
	MOVR sp, ilink
	rtie

/* separated entry for trap which may be used by irq_offload, USERPSACE */
SECTION_SUBSEC_FUNC(TEXT,__fault,__ev_trap)
	/* get the id of trap_s */
	LRR ilink, [_ARC_V2_ECR]
	ANDR ilink, ilink, 0x3f
#ifdef CONFIG_USERSPACE
	cmp ilink, _TRAP_S_CALL_SYSTEM_CALL
	bne _do_non_syscall_trap
	/* do sys_call */
	mov ilink, K_SYSCALL_LIMIT
	cmp r6, ilink
	blo valid_syscall_id

	mov_s r0, r6
	mov_s r6, K_SYSCALL_BAD

valid_syscall_id:
	/* create a sys call frame
	 * caller regs (r0 - 12) are saved in _create_irq_stack_frame
	 * ok to use them later
	 */
	_create_irq_stack_frame

	_save_exc_regs_into_stack

	/* exc return and do sys call in kernel mode,
	 * so need to clear U bit, r0 is already loaded
	 * with ERSTATUS in _save_exc_regs_into_stack
	 */

	bclr r0, r0, _ARC_V2_STATUS32_U_BIT
	sr r0, [_ARC_V2_ERSTATUS]

	mov_s r0, _arc_do_syscall
	sr r0, [_ARC_V2_ERET]

	rtie

_do_non_syscall_trap:
#endif /* CONFIG_USERSPACE */
	b _exc_entry
