/*
 * Copyright (c) 2020 Carlo Caione <ccaione@baylibre.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/syscall.h>
#include <zephyr/arch/arm64/mm.h>
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
 * int arch_buffer_validate(const void *addr, size_t size, int write)
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

2:	orr	x0, x0, #(MEM_DOMAIN_ALIGN_AND_SIZE - 1)
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
