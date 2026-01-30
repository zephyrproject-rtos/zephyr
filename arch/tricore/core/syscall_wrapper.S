/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/syscall.h>

GTEXT(_syscall_wrapper)
#if CONFIG_USERSPACE
GDATA(_k_syscall_table)
#endif

SECTION_FUNC(TEXT, _syscall_wrapper)
	/* Syscall table */
	jeq	%d15, 0, dispatch_syscall
	jeq	%d15, 1, switch_syscall
	jeq	%d15, 2, except_syscall
	j 	syscall_return

dispatch_syscall:
#if CONFIG_USERSPACE
	/* Check for invalid syscall number */
	mov 	%d13, lo:K_SYSCALL_BAD
	mov 	%d14, lo:K_SYSCALL_LIMIT
	ge	%d15, %d0, %d14
	cmov	%d0, %d15, %d13

	/* Allocate stack space for arg5 and arg6 to comply with EABI */
	sub.a	%a10, 8
	st.d	[%a10], %e8

	/* Load current lower context as argument for ssf */
	mfcr	%d15, TRICORE_PCXI
	sh	%d14, %d15, 12
	insert	%d14, %d14, %d15, 6, 16
	mov.a	%a4, %d14

	/* Load syscall handler from table and call it */
	movh.a	%a15, hi:_k_syscall_table
	lea	%a15, [%a15], lo:_k_syscall_table
	addsc.a	%a15, %a15, %d0, 2
	ld.a	%a14, [%a15]
	calli	%a14

	/* Restore stack */
	lea	%a10, [%a10], 8
#endif
	j syscall_return

except_syscall:
	mfcr  %d15, 0xFE00
	sh    %d5, %d15, 12
	insert %d5, %d5, %d15, 6, 16
	mov.a %a4, %d5
	call  z_tricore_fatal_error
	j syscall_return

switch_syscall:
	fcall	z_tricore_switch
	j syscall_return

syscall_return:
	rslcx
	rfe

