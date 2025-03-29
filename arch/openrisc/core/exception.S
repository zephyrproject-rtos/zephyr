/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <offsets_short.h>
#include <openrisc/openriscregs.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/arch/openrisc/syscall.h>
#include <zephyr/kernel_structs.h>
#include <zephyr/linker/sections.h>
#include <zephyr/toolchain.h>

#include "asm_macros.inc"

#define ESF_O(FIELD)	__struct_arch_esf_##FIELD##_OFFSET


/* imports */
GTEXT(z_openrisc_fault)

GTEXT(z_check_stack_sentinel)
GTEXT(z_get_next_switch_handle)
GTEXT(z_openrisc_handle_irqs)
GTEXT(z_openrisc_switch)
GTEXT(z_openrisc_timer_isr)

#if defined(CONFIG_SOC_RESET_HOOK)
GTEXT(soc_reset_hook)
#endif

/* exports */
GTEXT(__reset)
GTEXT(__start)

GTEXT(z_openrisc_thread_start)


/*
 * Zeros a continuous series of registers starting at first amd ending at last.
 */

.altmacro
.macro zero_regs first=0, last=31
	l.movhi r\first, 0
.if \last-\first
	zero_regs %first+1, \last
.endif
.endm


/*
 * Calls a given function, and sets the return address to return to another
 * location.
 */

.macro chain_call proc1, proc2
	l.movhi r9, hi(\proc2)
#ifdef __OR1K_NODELAY__
	l.ori r9, r9, lo(\proc2)
	l.j \proc1
#else
	l.j \proc1
	 l.ori r9, r9, lo(\proc2)
#endif
.endm


/*
 * Calls a function specified by a register, and sets the return address to
 * return to another location.
 */

.macro chain_call_reg proc1_reg, proc2
	l.movhi r9, hi(\proc2)
#ifdef __OR1K_NODELAY__
	l.ori r9, r9, lo(\proc2)
	l.jr \proc1_reg
#else
	l.jr \proc1_reg
	 l.ori r9, r9, lo(\proc2)
#endif
.endm


/*
 * Helper macro which loads or stores all caller-saved registers to the stack.
 */

.macro do_caller_saved op
	\op r3, ESF_O(r3), r1
	\op r4, ESF_O(r4), r1
	\op r5, ESF_O(r5), r1
	\op r6, ESF_O(r6), r1
	\op r7, ESF_O(r7), r1
	\op r8, ESF_O(r8), r1
	\op r11, ESF_O(r11), r1
	\op r12, ESF_O(r12), r1
	\op r13, ESF_O(r13), r1
	\op r15, ESF_O(r15), r1
	\op r17, ESF_O(r17), r1
	\op r19, ESF_O(r19), r1
	\op r21, ESF_O(r21), r1
	\op r23, ESF_O(r23), r1
	\op r25, ESF_O(r25), r1
	\op r27, ESF_O(r27), r1
	\op r29, ESF_O(r29), r1
	\op r31, ESF_O(r31), r1
.endm


/*
 * Stores all caller-saved registers to the stack.
 */

.macro store_caller_saved
	do_caller_saved op_store_reg
.endm


/*
 * Loads all caller-saved registers from the stack.
 */

.macro load_caller_saved
	do_caller_saved op_load_reg
.endm


/*
 * Stores the values of the ESR and EPCR registers into the est_t structure
 * pointed at by the r1 stack pointer.
 *
 * Modifies: r15
 */

.macro store_esr_epcr
	/* Save ESR */
	l.mfspr r15, r0, SPR_ESR_BASE
	l.sw ESF_O(esr)(r1), r15

	/* Save EPCR */
	l.mfspr r15, r0, SPR_EPCR_BASE
	l.sw ESF_O(epcr)(r1), r15
.endm


/*
 * Loads the values of the ESR and EPCR registers from the est_t structure
 * pointed at by the r1 stack pointer.
 *
 * Modifies: r15
 */

.macro load_esr_epcr
	/* Load ESR */
	l.lwz r15, ESF_O(esr)(r1)
	l.mtspr r0, r15, SPR_ESR_BASE

	/* Load EPCR */
	l.lwz r15, ESF_O(epcr)(r1)
	l.mtspr r0, r15, SPR_EPCR_BASE
.endm


/*
 * Stores the values of the MAC registers into the est_t structure pointed at
 * by the r1 stack pointer.
 *
 * Modifies: r15
 */

.macro store_mac
	l.mfspr r15, r0, SPR_MACLO
	l.sw ESF_O(mac_lo)(r1), r15
	l.mfspr r15, r0, SPR_MACHI
	l.sw ESF_O(mac_hi)(r1), r15
.endm


/*
 * Loads the values of the MAC registers from the est_t structure pointed at
 * by the r1 stack pointer.
 *
 * Modifies: r15
 */

.macro load_mac
	l.lwz r15, ESF_O(mac_lo)(r1)
	l.mtspr r0, r15, SPR_MACLO
	l.lwz r15, ESF_O(mac_hi)(r1)
	l.mtspr r0, r15, SPR_MACHI
.endm


/*
 * Declares a instruction exception label for a given address.
 */

.macro exception_label num, label
	.global _\label
	.org ((\num) * 0x100)
_\label\():

.endm


/*
 * Saves r9 on the current stack.
 */

.macro push_r9
	l.addi r1, r1, -4
	l.sw 0(r1), r9
.endm


/*
 * Restores r9 from the current stack.
 */

.macro pop_r9
	l.lwz r9, 0(r1)
	l.addi r1, r1, 4
.endm


/*
 * Increments or decrements the _cpu->nested counter.
 *
 * Modifies: value_reg
 */

.macro update_nested_count cpu_ptr_reg, value_reg, increment
	l.lwz \value_reg, ___cpu_t_nested_OFFSET(\cpu_ptr_reg)
	l.addi \value_reg, \value_reg, \increment
	l.sw ___cpu_t_nested_OFFSET(\cpu_ptr_reg), \value_reg
.endm


/*
 * Gets the current CPU structure.
 * Loads the IRQ stack pointer, and updates the nested IRQ counter.
 *
 * Modifies: dst
 */

.macro get_current_cpu dst
	l.movhi \dst, hi(_kernel + ___kernel_t_cpus_OFFSET)
	l.ori \dst, \dst, lo(_kernel + ___kernel_t_cpus_OFFSET)
.endm


/*
 * Loads the IRQ stack pointer, and updates the nested IRQ counter.
 *
 * Modifies: r15, r17
 */

.macro setup_irq_stack
	/* Get the cpu_t pointer */
	get_current_cpu r15

	/* Increment the nested IRQ counter */
	update_nested_count r15, r17, 1

	/* Skip if we're already in an interrupt, and save the stack pointer */
	l.sfgtui r17, 1
#ifdef __OR1K_NODELAY__
	l.ori r17, r1, 0
	l.bf 1f
#else
	l.bf 1f
	 l.ori r17, r1, 0
#endif

	/* Switch to interrupt stack */
	l.lwz r1, ___cpu_t_irq_stack_OFFSET(r15)

	/* Save the stack pointer on the IRQ stack */
	l.addi r1, r1, -4
	l.sw 0(r1), r17

1:
.endm


/*
 * Common head code required by all exception handler.
 */

.macro exception_handler_head
	/* Allocate esf_t on the stack */
	l.addi r1, r1, lo(-__struct_arch_esf_SIZEOF)

	/* Store caller-saved, exception and MAC registers */
	store_caller_saved
	store_esr_epcr
	store_mac
.endm


/*
 * Common head code required by all ISRs.
 */

.macro isr_head
	exception_handler_head

	setup_irq_stack

	/* Save r9 on the stack */
	push_r9
.endm


/*
 * Common error handler functionality.
 */

.macro error_handler_tail num
	/* Call z_openrisc_fault with the esf and reason arguments */
	l.ori r3, r1, 0
#ifdef __OR1K_NODELAY__
	l.ori r4, r0, \num
	l.jal z_openrisc_fault
#else
	l.jal z_openrisc_fault
	 l.ori r4, r0, \num
#endif

	/* Halt */
	l.j .
#ifndef __OR1K_NODELAY__
	 l.nop
#endif

.endm


/*
 * Implements system initialization on reset.
 */

.macro exception_reset
SECTION_SUBSEC_FUNC(exceptions, _reset_and__start, __reset)
SECTION_SUBSEC_FUNC(exceptions, _reset_and__start, __start)
exception_label 0x1, reset
	/*
	 * Initialize Supervision Register:
	 * Supervisor mode on, all interrupts off, caches off
	 */
	l.ori r14, r0, SPR_SR_SM
	l.mtspr r0, r14, SPR_SR

	/* Zero all registers */
	zero_regs 1, 31

#if defined(CONFIG_SOC_RESET_HOOK)
	l.jal soc_reset_hook
#ifndef __OR1K_NODELAY__
	 l.nop
#endif
#endif

#ifdef CONFIG_INIT_STACKS
	/* Pre-populate all bytes in z_interrupt_stacks with 0xAA */
	l.movhi r14, hi(z_interrupt_stacks)
	l.ori r14, r14, lo(z_interrupt_stacks)
	l.addi r16, r14, __z_interrupt_all_stacks_SIZEOF

	l.movhi r16, 0xAAAA
	l.ori r16, r16, 0xAAAA

aa_loop:
	l.sw 0(r14), r16
	l.addi r14, r14, 4
	l.sfltu r14, r16
	l.bf aa_loop
#ifndef __OR1K_NODELAY__
	 l.nop
#endif

#endif /* CONFIG_INIT_STACKS */

	/* Setup stack pointer. */
	l.movhi r1, hi(__stack)
	l.ori r1, r1, lo(__stack)

	/* Set frame pointer */
	l.ori r2, r1, 0

	/* Jump into C domain. */
	l.j z_prep_c
#ifndef __OR1K_NODELAY__
	 l.nop
#endif

.endm


/*
 * Handles exception errors
 */

.macro exception_error num, label
exception_label \num, \label
	exception_handler_head
	error_handler_tail \num
.endm


/*
 * Handles Tick Timer Exception.
 *
 * Saves register state, then implements tick-timer handling functionality
 * then hands off to _isr_tail to implemented shared tail functionality.
 */

.macro exception_tick_timer
exception_label 0x5, tick_timer
	isr_head

	/* Handle the tick timer and jump to _isr_tail on return. */
	chain_call z_openrisc_timer_isr, _isr_tail
.endm


/*
 * Handles External Interrupt Exception.
 *
 * Saves register state, then implements interrupt handling functionality
 * then hands off to _isr_tail to implemented shared tail functionality.
 */

.macro exception_external_interrupt
exception_label 0x8, external_interrupt
	isr_head

	/* Call to z_openrisc_handle_irqs and return to _isr_tail. */
	chain_call z_openrisc_handle_irqs, _isr_tail
.endm


/*
 * Handles syscalls
 */

.macro exception_syscall
exception_label 0xC, syscall
	exception_handler_head

#ifdef CONFIG_IRQ_OFFLOAD
	/* Determine what to do. Operation code is in r11 */
	l.sfeqi r11, OR_SYSCALL_IRQ_OFFLOAD
	l.bf do_irq_offload
#ifndef __OR1K_NODELAY__
	 l.nop
#endif

#endif

	error_handler_tail 0xC

#ifdef CONFIG_IRQ_OFFLOAD
do_irq_offload:
	setup_irq_stack

	/* Save r9 on the stack */
	push_r9

	/*
	 * Save r9, invoke the offload handler and jump to _isr_tail on
	 * return.
	 */
	l.ori r15, r3, 0
	l.ori r3, r4, 0
	chain_call_reg r15, _isr_tail

#endif
.endm


SECTION_FUNC(exceptions, _exceptions)

/* Reset */
exception_reset

/* Bus Error */
exception_error 0x2, bus_error

/* Data Page Fault */
exception_error 0x3, data_page_fault

/* Instruction Page Fault */
exception_error 0x4, instruction_page_fault

/* Tick Timer */
exception_tick_timer

/* Alignment Exception */
exception_error 0x6, alignment_exception

/* Illegal Instruction */
exception_error 0x7, illegal_instruction

/* External Interrupt */
exception_external_interrupt

/* Data TLB Miss */
exception_error 0x9, dtlb_miss

/* Instruction TLB Miss */
exception_error 0xA, itlb_miss

/* Range Exception */
exception_error 0xB, range_exception

/* System Call */
exception_syscall

/* Floating Point Exception */
exception_error 0xD, floating_point_exception

/* Trap */
exception_error 0xE, trap_exception


/*
 * Common interrupt handler tail routine.
 *
 * This routing implements common interrupt handler tail behaviour, shared
 * between the tick timer and external exceptions and exits out of exception
 * context when complete.
 */

SECTION_FUNC(exceptions, _isr_tail)
	/* Get the cpu_t pointer */
	get_current_cpu r15

	/* Decrement the nested IRQ count */
	update_nested_count r15, r17, -1

	/* If the nested count is back to zero, restore the thread stack */
	l.sfnei r17, 0
	l.bf retain_irq_stack
#ifndef __OR1K_NODELAY__
	 l.nop
#endif

	/* Restore r9 */
	pop_r9

        /* Restore stack pointer */
        l.lwz r1, 0(r1)

retain_irq_stack:
	/* Save temporary registers */
#if defined(CONFIG_MULTITHREADING)
	l.addi r1, r1, -8
	l.sw 0(r1), r9
	l.sw 4(r1), r14
#elif defined(CONFIG_STACK_SENTINEL)
	l.addi r1, r1, -4
	l.sw 0(r1), r9
#endif

#ifdef CONFIG_STACK_SENTINEL
	l.jal z_check_stack_sentinel
#ifndef __OR1K_NODELAY__
	 l.nop
#endif

#endif

check_reschedule:
	/*
	 * Load the address of the current k_thread from the current cpu_t for use by
	 * z_openrisc_switch
	 */
	get_current_cpu r15
	l.lwz r14, ___cpu_t_current_OFFSET(r15)

#ifdef CONFIG_MULTITHREADING
	/* Call z_get_next_switch_handle(NULL) */
#ifdef __OR1K_NODELAY__
	l.ori r3, r0, 0
	l.jal z_get_next_switch_handle
#else
	l.jal z_get_next_switch_handle
	 l.ori r3, r0, 0
#endif

	/* If the return handle was NULL, exit the ISR without rescheduling */
	l.sfeqi r11, 0
	l.bf no_reschedule
#ifndef __OR1K_NODELAY__
	 l.nop
#endif

reschedule:
	/*
	 * If the handle was not NULL, call
	 * z_openrisc_context_switch(handle, prev_thread).
	 * where esf is currently pointed to by the stack pointer.
	 */
	l.ori r3, r11, 0
#ifdef __OR1K_NODELAY__
	l.ori r4, r14, 0
	l.jal z_openrisc_switch
#else
	l.jal z_openrisc_switch
	 l.ori r4, r14, 0
#endif

no_reschedule:

#endif

	/* Restore temporary registers */
#if defined(CONFIG_MULTITHREADING)
	l.lwz r9, 0(r1)
	l.lwz r14, 4(r1)
	l.addi r1, r1, 8
#elif defined(CONFIG_STACK_SENTINEL)
	l.lwz r9, 0(r1)
	l.addi r1, r1, 4
#endif

SECTION_FUNC(exceptions, z_openrisc_thread_start)
	/* Reload caller-saved, exception and MAC registers */
	load_caller_saved
	load_mac
	load_esr_epcr

	/* Restore stack pointer */
	l.addi r1, r1, lo(__struct_arch_esf_SIZEOF)

	/* Return from exception */
	l.rfe
