/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/kernel.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/sw_isr_table.h>
#include <offsets_short.h>

GDATA(_kernel)
GDATA(_sw_isr_table)
GTEXT(_isr_wrapper)
GTEXT(z_soc_irq_get_active)
#if CONFIG_MULTITHREADING
GTEXT(z_get_next_switch_handle)
GTEXT(z_tricore_switch)
#endif
#ifdef CONFIG_TRACING_ISR
GTEXT(sys_trace_isr_enter)
GTEXT(sys_trace_isr_exit)
#endif

/**
 *
 * @brief Wrapper around ISRs when inserted in software ISR table
 *
 * When inserted in the vector table, _isr_wrapper() demuxes the ISR table
 * using the running interrupt number as the index, and invokes the registered
 * ISR with its corresponding argument. When returning from the ISR, it
 * determines if a context switch needs to happen from next IRQ register.
 *
 */
SECTION_FUNC(TEXT, _isr_wrapper)
	/* Get cpu structure base */
	movh.a  %a15, hi:(_kernel + ___kernel_t_cpus_OFFSET)
	lea     %a15, [%a15], lo:(_kernel + ___kernel_t_cpus_OFFSET)

	ld.w %d15, [%a15]+___cpu_t_nested_OFFSET
	addi %d15, %d15, 1
	st.w [%a15]+___cpu_t_nested_OFFSET, %d15

#ifdef CONFIG_TRACING_ISR
	call sys_trace_isr_enter
#endif

	/* Get current irq number */
	call	intc_aurix_ir_get_active

	/* Load isr table entry */
	movh.a	%a13, hi:_sw_isr_table
	lea	%a13, [%a13], lo:_sw_isr_table
	addsc.a	%a13, %a13, %d2, 3

	/* Load arg and function ptr */
#if defined(__clang__)
        ld.da   %p4, [%a13]
#else
        ld.da   %a4, [%a13]
#endif

	/* Call ISR */
	calli	%a5

#ifdef CONFIG_TRACING_ISR
	call	sys_trace_isr_exit
#endif

	ld.w %d15, [%a15]+___cpu_t_nested_OFFSET
	addi %d15, %d15, -1
	st.w [%a15]+___cpu_t_nested_OFFSET, %d15

#if defined(CONFIG_MULTITHREADING)
	/* Check nesting */
	jnz	%d15, no_reschedule

	/* Check pending irq for reschedule */
	mfcr	%d15, TRICORE_ICR
	extr.u	%d15, %d15, 16, 8
	jnz	%d15, no_reschedule

	/* Load current thread */
	ld.a	%a14, [%a15]+___cpu_t_current_OFFSET

	/* Get next thread to switch to */
	mov.aa	%a4, %a14
	call	z_get_next_switch_handle
	jz.a	%a2, no_reschedule

	/* Perform thread switch */
	mov.aa	%a4, %a2
	mov.aa	%a5, %a14
	fcall	z_tricore_switch
#endif

no_reschedule:
	/* Return from interrupt */
	rslcx
	rfe

