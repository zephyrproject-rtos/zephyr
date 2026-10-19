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

#if defined(CONFIG_SYSCALL_RETURN_HOOK)
GTEXT(z_sig_handle)
#endif

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

	mrs	x3, DAIF
	adds	x1, x1, x0
	b.cs	abv_fail
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
	/*
	 * CFI: entered from z_arm64_sync_exc with SP at struct arch_esf.
	 * Needed so GDB can unwind from syscall implementations.
	 */
	.cfi_sections .debug_frame
	.cfi_startproc
	.cfi_def_cfa sp, ___esf_t_SIZEOF
	.cfi_offset 30, (___esf_t_spsr_elr_OFFSET + 8 - ___esf_t_SIZEOF)
#ifdef CONFIG_FRAME_POINTER
	.cfi_offset 29, (___esf_t_fp_OFFSET - ___esf_t_SIZEOF)
#endif

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

#ifdef CONFIG_SYSCALL_RETURN_HOOK
	ldr	x0, =K_SYSCALL_K_SIG_ACTION
	cmp	x0, x8
	beq skip_k_sig_handle
	ldr	x0, =K_SYSCALL_K_SIG_CURRENT
	cmp	x0, x8
	beq skip_k_sig_handle
	ldr	x0, =K_SYSCALL_K_SIG_RETURN
	cmp	x0, x8
	beq z_sig_return_restore

	mov	x0, sp
	ldr	x9, =z_sig_handle
	blr	x9
	cbz	x0, skip_k_sig_handle

	/*
	 * z_sig_handle() redirected the return path into the signal trampoline. The trampoline
	 * chain clobbers the callee-saved registers per the ABI and k_sig_return() resumes the
	 * interrupted context without unwinding, so capture them now: at this point they still
	 * hold the interrupted context's values.
	 */
	get_cpu	x9
	ldr	x9, [x9, #___cpu_t_current_OFFSET]
	ldr	x10, =__thread_sig_user_x_OFFSET
	add	x9, x9, x10
	stp	x19, x20, [x9, #0]
	stp	x21, x22, [x9, #16]
	stp	x23, x24, [x9, #32]
	stp	x25, x26, [x9, #48]
	stp	x27, x28, [x9, #64]
	b	skip_k_sig_handle

z_sig_return_restore:
	get_cpu	x9
	ldr	x9, [x9, #___cpu_t_current_OFFSET]

	/* the return-value store above overwrote the resumed context's x0 */
	ldr	x10, =__thread_sig_user_x0_OFFSET
	ldr	x10, [x9, x10]
	str	x10, [sp, ___esf_t_x0_x1_OFFSET]

	/* restore the callee-saved registers captured when the trampoline was entered */
	ldr	x10, =__thread_sig_user_x_OFFSET
	add	x9, x9, x10
	ldp	x19, x20, [x9, #0]
	ldp	x21, x22, [x9, #16]
	ldp	x23, x24, [x9, #32]
	ldp	x25, x26, [x9, #48]
	ldp	x27, x28, [x9, #64]

skip_k_sig_handle:
#endif

	/* Return from exception */
	.cfi_endproc
	b	z_arm64_exit_exc
