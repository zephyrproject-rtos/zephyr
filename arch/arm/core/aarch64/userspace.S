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
