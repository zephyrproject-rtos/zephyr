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

	/* Use the ESF as SSF */
	mov	x6, sp

	/* Recover the syscall ID */
	ldr	x8, [sp, ___esf_t_x8_x9_OFFSET]

	/* Check whether the ID is valid */
	ldr	x9, =K_SYSCALL_LIMIT
	cmp	x8, x9
	blo	valid_syscall_id

	/* Save the bad ID for handler_bad_syscall() */
	mov	x0, x8
	ldr	x8, =K_SYSCALL_BAD

valid_syscall_id:
	ldr	x9, =_k_syscall_table
	ldr	x9, [x9, x8, lsl #3]

	/* Jump into the syscall */
	msr	daifclr, #(DAIFSET_IRQ_BIT)
	blr	x9
	msr	daifset, #(DAIFSET_IRQ_BIT)

	/* Save the return value into the ESF */
	str	x0, [sp, ___esf_t_x0_x1_OFFSET]

	/* Return from exception */
	b	z_arm64_exit_exc

/*
 * Routine to jump into userspace
 *
 * We leverage z_arm64_exit_exc() to pop out the entry function and parameters
 * from ESF and fake a return from exception to move from EL1 to EL0. The fake
 * ESF is built in arch_user_mode_enter() before jumping here.
 */

GTEXT(z_arm64_userspace_enter)
SECTION_FUNC(TEXT, z_arm64_userspace_enter)

	msr	DAIFSET, #DAIFSET_IRQ_BIT

	/*
	 * When a kernel thread is moved to user mode it doesn't have any
	 * SP_EL0 set yet. We set it here for the first time pointing to the
	 * beginning of the user accessible part of the stack (the top).
	 */
	msr	sp_el0, x1

	/*
	 * Set SP_EL1 to point at the end of the ESF. Since we have relocated
	 * the ESF at the beginning of the privileged stack area, when the ESF
	 * is popped out by z_arm64_exit_exc() the SP_EL1 will be at the right
	 * location for when the next exception will come.
	 */
	mov	sp, x0

	/* we have to fake our exception depth count too */
	mrs	x0, tpidrro_el0
	mov	x1, #TPIDRROEL0_EXC_UNIT
	bic	x0, x0, #TPIDRROEL0_EXC_DEPTH
	add	x0, x0, x1
	msr	tpidrro_el0, x0

	b	z_arm64_exit_exc
