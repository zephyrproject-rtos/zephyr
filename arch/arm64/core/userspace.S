/*
 * Copyright (c) 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <toolchain.h>
#include <linker/sections.h>
#include <offsets_short.h>
#include <arch/cpu.h>
#include <syscall.h>
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

/*
 * size_t arch_user_string_nlen(const char *s, size_t maxsize, int *err_arg)
 */

GTEXT(z_arm64_user_string_nlen_fault_start)
GTEXT(z_arm64_user_string_nlen_fault_end)
GTEXT(z_arm64_user_string_nlen_fixup)

GTEXT(arch_user_string_nlen)
SECTION_FUNC(TEXT, arch_user_string_nlen)

	mov	x3, x0
	mov	x0, #0
	mov	x4, #0

strlen_loop:

	cmp	x0, x1
	beq	strlen_done

z_arm64_user_string_nlen_fault_start:
	ldrb	w5, [x3, x0]
z_arm64_user_string_nlen_fault_end:
	cbz	x5, strlen_done

	add	x0, x0, #1
	b	strlen_loop

z_arm64_user_string_nlen_fixup:
	mov	x4, #-1
	mov	x0, #0

strlen_done:
	str	w4, [x2]
	ret

/*
 * int arch_buffer_validate(void *addr, size_t size, int write)
 */

GTEXT(arch_buffer_validate)
SECTION_FUNC(TEXT, arch_buffer_validate)

	add	x1, x1, x0
	mrs	x3, DAIF
	msr	DAIFSET, #DAIFSET_IRQ_BIT

abv_loop:
	cbnz	w2, 1f
	at	S1E0R, x0
	b	2f
1:	at	S1E0W, x0

2:	orr	x0, x0, #(CONFIG_MMU_PAGE_SIZE - 1)
	add	x0, x0, #1

	isb
	mrs	x4, PAR_EL1
	tbnz	x4, #0, abv_fail

	cmp	x0, x1
	blo	abv_loop

	msr	DAIF, x3
	mov	x0, #0
	ret

abv_fail:
	msr	DAIF, x3
	mov	x0, #-1
	ret

/*
 * System call entry point.
 */

GTEXT(z_arm64_do_syscall)
SECTION_FUNC(TEXT, z_arm64_do_syscall)
	/* Recover the syscall parameters from the ESF */
	ldp	x0, x1, [sp, ___esf_t_x0_x1_OFFSET]
	ldp	x2, x3, [sp, ___esf_t_x2_x3_OFFSET]
	ldp	x4, x5, [sp, ___esf_t_x4_x5_OFFSET]

	/* Recover the syscall ID */
	ldr	x8, [sp, ___esf_t_x8_x9_OFFSET]

	/* Check whether the ID is valid */
	ldr	x9, =K_SYSCALL_LIMIT
	cmp	x8, x9
	blo	valid_syscall_id
	ldr	x8, =K_SYSCALL_BAD

valid_syscall_id:
	ldr	x9, =_k_syscall_table
	ldr	x9, [x9, x8, lsl #3]

	/* Recover the privileged stack */
#ifdef CONFIG_SMP
	get_cpu	x10, x8
	ldr	x10, [x10, #___cpu_t_current_OFFSET]
#else
	ldr	x10, =_kernel
	ldr	x10, [x10, #_kernel_offset_to_current]
#endif
	ldr     x10, [x10, #_thread_offset_to_priv_stack_start]
	add	x10, x10, #CONFIG_PRIVILEGED_STACK_SIZE

	/* Save the original SP on the privileged stack */
	mov	x11, sp
	mov	sp, x10
	str	x11,  [sp, #-16]!

	/* Jump into the syscall */
	msr	daifclr, #(DAIFSET_IRQ_BIT)
	blr	x9
	msr	daifset, #(DAIFSET_IRQ_BIT)

	/* Restore the original SP containing the ESF */
	ldr	x11, [sp], #16
	mov	sp, x11

	/* Save the return value into the ESF */
	str	x0, [sp, ___esf_t_x0_x1_OFFSET]

	/* Return from exception */
	b	z_arm64_exit_exc

/*
 * Routine to jump into userspace
 *
 * We leverage z_arm64_exit_exc() to pop out the entry function and parameters
 * from ESF and fake a return from exception to move from EL1 to EL0. The fake
 * ESF is built in arch_user_mode_enter() before jumping here
 */

GTEXT(z_arm64_userspace_enter)
SECTION_FUNC(TEXT, z_arm64_userspace_enter)
	mov	sp, x0
	b	z_arm64_exit_exc
