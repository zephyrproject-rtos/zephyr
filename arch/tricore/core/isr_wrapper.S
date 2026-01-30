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

GTEXT(z_tricore_irq_done)
z_tricore_irq_done:

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

	/* Perform thread switch.
	 *
	 * On an ISR self-abort, kernel/sched.c halt_thread() has dummified
	 * _current (cpus[0].current = &_thread_dummy) before this point.
	 * The live CSA chain still belongs to the aborting thread, which
	 * the kernel reaches via to_reclaim. Saving that chain into the
	 * dummy would leave the aborting thread's callee_saved.pcxi stale,
	 * and the subsequent z_tricore_reclaim_csa walk would follow the
	 * stale (long-recycled) chain into other threads' CSAs and strip
	 * their UL/PIE bits -- which then traps Class 3 TIN 6 at the rfe
	 * below. Use to_reclaim as the outgoing thread when set so the
	 * chain saved matches the chain to be reclaimed.
	 */
	mov.aa	%a4, %a2
	ld.a	%a13, [%a15]+_cpu_offset_to_to_reclaim
	jz.a	%a13, 1f
	mov.aa	%a5, %a13
	j	2f
1:
	mov.aa	%a5, %a14
2:
	fcall	z_tricore_switch

	/* Reclaim CSAs if needed */
	ld.a	%a13, [%a15]+_cpu_offset_to_to_reclaim
	jz.a	%a13, no_reschedule
	mov.aa	%a4, %a13
	call	z_tricore_reclaim_csa
	mov.a	%a12, 0
	st.a	[%a15]+_cpu_offset_to_to_reclaim, %a12
	st.a	[%a13]+_thread_offset_to_saved_pcxi, %a12
#endif

no_reschedule:
	/* Return from interrupt */
	rslcx
	rfe
