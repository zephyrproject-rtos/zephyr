/*
 * Copyright (c) 2016-2026 Vinayak Kariappa Chettimada
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARM Cortex-M nanobe context switch and ISR injection
 *
 * Nanobes run in thread mode on the process stack of the nanobe owner thread.
 * A context switch only saves the AAPCS callee-saved registers and the
 * injection guard of the outgoing context on its own stack, and swaps the
 * stack pointer. The frame layout must match struct nanobe_frame in
 * kernel/nanobe.c:
 *
 *   high address:  lr (resume pc)
 *                  r7, r6, r5, r4
 *                  r11, r10, r9, r8
 *                  s31 .. s16          (CONFIG_NANOBE_FPU only)
 *   saved sp ->    injection guard
 *
 * The context switch is ARMv6-M (Thumb-1) compatible, the injection syringe
 * requires ARMv7-M/ARMv8-M Mainline.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(nanobe_switch)
GTEXT(z_nanobe_entry)
GTEXT(z_nanobe_exit)

GDATA(z_nanobe_sgrd)
GDATA(z_nanobe_igrd)

/*
 * void nanobe_switch(void *next_sp, void **curr_sp);
 */
SECTION_FUNC(TEXT, nanobe_switch)
	/* save callee-saved registers of the current context */
	push {r4-r7, lr}
	mov r4, r8
	mov r5, r9
	mov r6, r10
	mov r7, r11
	push {r4-r7}
#if defined(CONFIG_NANOBE_FPU)
	vpush {s16-s31}
#endif /* CONFIG_NANOBE_FPU */

	/* save the injection guard of the current context and block
	 * injections while the stack is being swapped.
	 */
	ldr r3, =z_nanobe_igrd
	ldrb r2, [r3]
	push {r2}
	movs r2, #1
	strb r2, [r3]

	/* swap the stack */
	mov r2, sp
	str r2, [r1]
	mov sp, r0

	/* restore the injection guard of the next context */
	pop {r2}
	strb r2, [r3]

	/* release the nanobe scheduler lock */
	ldr r3, =z_nanobe_sgrd
	movs r2, #0
	strb r2, [r3]

	/* restore callee-saved registers of the next context */
#if defined(CONFIG_NANOBE_FPU)
	vpop {s16-s31}
#endif /* CONFIG_NANOBE_FPU */
	pop {r4-r7}
	mov r8, r4
	mov r9, r5
	mov r10, r6
	mov r11, r7
	pop {r4-r7, pc}

/*
 * Nanobe entry trampoline, first resumed by nanobe_switch() with the nanobe
 * entry function in r4 and its argument in r5.
 */
SECTION_FUNC(TEXT, z_nanobe_entry)
	mov r0, r5
	blx r4
	/* nanobe entry function must not return */
	bl z_nanobe_exit

GTEXT(z_nanobe_syringe)

GDATA(z_nanobe_itrg)
GDATA(z_nanobe_iret)
GDATA(z_nanobe_ical)

/*
 * Injection syringe.
 *
 * nanobe_isr_inject() redirects the exception return address of the
 * interrupted owner thread here. Executes in thread mode with the
 * interrupted context's registers, calls the injected callee preserving all
 * the caller-saved state (r0-r3, r12, lr, APSR and, if the FP context is
 * active, s0-s15 and FPSCR) and resumes the interrupted code.
 */
SECTION_FUNC(TEXT, z_nanobe_syringe)
	/* reserve slot for the interrupted return address */
	push {r0}
	push {r0-r3, r12, lr}
	ldr r1, =z_nanobe_iret
	ldr r1, [r1]
	str r1, [sp, #24]

	/* save APSR and the unaligned sp, align sp to 8 bytes for AAPCS */
	mrs r0, apsr
	mov r1, sp
	bic r2, r1, #7
	mov sp, r2
	push {r0, r1}

#if defined(CONFIG_NANOBE_FPU)
	/* save caller-saved FP registers if the FP context is active */
	mrs r2, control
	ands r2, r2, #4 /* CONTROL.FPCA */
	beq 1f
	vmrs r3, fpscr
	vpush {s0-s15}
1:
	push {r2, r3}
#endif /* CONFIG_NANOBE_FPU */

2:
	ldr r0, =z_nanobe_ical
	ldr r0, [r0]
	blx r0

	/* injection done, re-run if re-injected meanwhile */
	ldr r1, =z_nanobe_igrd
	movs r0, #0
	strb r0, [r1]
	ldr r2, =z_nanobe_itrg
	ldrb r0, [r2]
	cbz r0, 3f
	movs r0, #0
	strb r0, [r2]
	movs r0, #1
	strb r0, [r1]
	b 2b

3:
#if defined(CONFIG_NANOBE_FPU)
	pop {r2, r3}
	cbz r2, 4f
	vpop {s0-s15}
	vmsr fpscr, r3
4:
#endif /* CONFIG_NANOBE_FPU */

	pop {r0, r1}
	mov sp, r1
#if defined(__ARM_FEATURE_DSP)
	msr apsr_nzcvqg, r0
#else
	msr apsr_nzcvq, r0
#endif
	pop {r0-r3, r12, lr}
	pop {pc}

