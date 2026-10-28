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
#if CONFIG_USERSPACE
GDATA(_k_syscall_table)
#endif
#if CONFIG_IRQ_OFFLOAD
GTEXT(z_tricore_irq_done)
#endif
#if CONFIG_MPU_STACK_GUARD
GTEXT(z_tricore_mpu_stackguard_enable)
#endif

SECTION_FUNC(TEXT, _syscall_wrapper)
	/* Syscall table */
#if CONFIG_USERSPACE
	jeq	%d15, TRICORE_SYSCALL_CALL, dispatch_syscall
#endif
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

	/* z_tricore_switch clobbers %a15 across the MPU reconfigure */
	movh.a	%a15, hi:(_kernel + ___kernel_t_cpus_OFFSET)
	lea	%a15, [%a15], lo:(_kernel + ___kernel_t_cpus_OFFSET)

	ld.a	%a13, [%a15]+_cpu_offset_to_to_reclaim
	jz.a	%a13, switch_no_reclaim
	mov.aa	%a4, %a13
	call	z_tricore_reclaim_csa
	mov.a	%a12, 0
	st.a	[%a15]+_cpu_offset_to_to_reclaim, %a12
	st.a	[%a13]+_thread_offset_to_saved_pcxi, %a12
switch_no_reclaim:
#if CONFIG_MPU_STACK_GUARD
	/* After thread switch, install the new thread's stack guard */
	movh.a  %a15, hi:(_kernel + ___kernel_t_cpus_OFFSET)
	lea     %a15, [%a15], lo:(_kernel + ___kernel_t_cpus_OFFSET)
	ld.a    %a4, [%a15]+___cpu_t_current_OFFSET
	call    z_tricore_mpu_stackguard_enable
#endif
	j 	syscall_return

#if CONFIG_USERSPACE
dispatch_syscall:
	/* Check for invalid syscall number */
	mov 	%d13, lo:K_SYSCALL_BAD
	mov 	%d14, lo:K_SYSCALL_LIMIT
	ge	%d15, %d0, %d14
	cmov	%d0, %d15, %d13

	/* Run the handler on the privileged stack of the current thread */
	movh.a	%a15, hi:(_kernel + ___kernel_t_cpus_OFFSET)
	lea	%a15, [%a15], lo:(_kernel + ___kernel_t_cpus_OFFSET)
	ld.a	%a15, [%a15]+___cpu_t_current_OFFSET
	ld.a	%a10, [%a15]+_thread_offset_to_priv_stack_end

	/* Allocate stack space for arg5 and arg6 to comply with EABI */
	sub.a	%a10, 8
	st.d	[%a10], %e8

	/* Lower context address from PCXI */
	mfcr	%d15, TRICORE_PCXI
	extr.u	%d14, %d15, 16, 4
	sh	%d14, %d14, 28
	insert	%d14, %d14, %d15, 6, 16
	mov.a	%a4, %d14

	/* Load syscall handler from table */
	movh.a	%a15, hi:_k_syscall_table
	lea	%a15, [%a15], lo:_k_syscall_table
	addsc.a	%a15, %a15, %d0, 2
	ld.a	%a14, [%a15]

	/* Restore the caller's interrupt state from the saved PCXI.PIE */
	ld.w	%d15, [%a4]
	jz.t	%d15, 21, 1f
	enable
1:
	calli	%a14

	/* Restore stack */
	lea	%a10, [%a10], 8

	/* rslcx restores D2, so carry the return value in D15 */
	mov	%d15, %d2
	rslcx
	mov	%d2, %d15
	rfe
#endif

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
	/* Filters the reason raised from user mode */
	call  	z_tricore_user_except

syscall_return:
	rslcx
	rfe
