/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 Infineon Technologies AG,
 * SPDX-FileCopyrightText: or an affiliate of Infineon Technologies AG. All rights reserved.
 * SPDX-FileCopyrightText: Copyright (c) 2026 Linumiz
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/syscall.h>
#include <offsets_short.h>

GTEXT(_syscall_wrapper)
GTEXT(z_tricore_reclaim_csa)
GDATA(_kernel)
#if CONFIG_IRQ_OFFLOAD
GTEXT(z_tricore_irq_done)
#endif

SECTION_FUNC(TEXT, _syscall_wrapper)
	/* Syscall table */
	jeq	%d15, TRICORE_SYSCALL_SWITCH, switch_syscall
	jeq	%d15, TRICORE_SYSCALL_EXCEPT, except_syscall
#if CONFIG_IRQ_OFFLOAD
	jeq	%d15, TRICORE_SYSCALL_IRQ_OFFLOAD, irq_offload
#endif
	j 	invalid_syscall

switch_syscall:
	movh.a	%a15, hi:(_kernel + ___kernel_t_cpus_OFFSET)
	lea	%a15, [%a15], lo:(_kernel + ___kernel_t_cpus_OFFSET)

	ld.a	%a13, [%a15]+_cpu_offset_to_to_reclaim
	jz.a	%a13, 1f
	mov.aa	%a5, %a13
1:
	fcall	z_tricore_switch

	ld.a	%a13, [%a15]+_cpu_offset_to_to_reclaim
	jz.a	%a13, switch_no_reclaim
	mov.aa	%a4, %a13
	call	z_tricore_reclaim_csa
	mov.a	%a12, 0
	st.a	[%a15]+_cpu_offset_to_to_reclaim, %a12
	st.a	[%a13]+_thread_offset_to_saved_pcxi, %a12
switch_no_reclaim:
	j 	syscall_return

#if CONFIG_IRQ_OFFLOAD
irq_offload:
	/* Get cpu structure base */
	movh.a  %a15, hi:(_kernel + ___kernel_t_cpus_OFFSET)
	lea     %a15, [%a15], lo:(_kernel + ___kernel_t_cpus_OFFSET)

	ld.w %d15, [%a15]+___cpu_t_nested_OFFSET
	addi %d15, %d15, 1
	st.w [%a15]+___cpu_t_nested_OFFSET, %d15

	calli	%a5
	j	z_tricore_irq_done
#endif

invalid_syscall:
	/* Set K_ERR_CPU_EXCEPTION as reason */
	mov	%d4, 0
except_syscall:
	mfcr  	%d15, TRICORE_PCXI
	sh    	%d5, %d15, 12
	insert	%d5, %d5, %d15, 6, 16
	mov.a 	%a4, %d5
	call  	z_tricore_fatal_error

syscall_return:
	rslcx
	rfe
