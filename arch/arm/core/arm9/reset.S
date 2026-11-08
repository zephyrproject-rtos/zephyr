/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief ARMv5 reset handler
 *
 * Brings the core into a known state (MMU and caches off, caches and TLBs
 * invalidated), installs one stack per processor mode and hands over to
 * z_prep_c().
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <offsets_short.h>
#include "vector_table.h"
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

GTEXT(z_arm_reset)
GDATA(z_arm_svc_stack)
GDATA(z_arm_sys_stack)
GDATA(z_arm_fiq_stack)
GDATA(z_arm_abort_stack)
GDATA(z_arm_undef_stack)
#if defined(CONFIG_SOC_RESET_HOOK)
GTEXT(soc_reset_hook)
#endif

/*
 * Each exception mode keeps a three word scratch area at the very top of its
 * stack; z_armv5_enter_exc() parks r0, lr_<exc> and spsr_<exc> there.
 */
#define EXC_SCRATCH_SIZE	12

SECTION_SUBSEC_FUNC(TEXT, _reset_section, z_arm_reset)
SECTION_SUBSEC_FUNC(TEXT, _reset_section, __start)

	/*
	 * Make sure we are in SVC mode with both IRQ and FIQ masked, whatever
	 * the boot loader left behind.
	 */
	msr	cpsr_c, #(MODE_SVC | I_BIT | F_BIT)

	/*
	 * Turn the MMU, the caches and the branch predictor off. Zephyr sets
	 * these up itself later on; a boot loader (or a warm reset) may well
	 * have left them enabled with stale translations.
	 */
	mrc	p15, 0, r0, c1, c0, 0
	bic	r0, r0, #(SCTLR_M_BIT | SCTLR_C_BIT | SCTLR_A_BIT)
	bic	r0, r0, #SCTLR_I_BIT
	bic	r0, r0, #SCTLR_Z_BIT
	bic	r0, r0, #SCTLR_V_BIT
	mcr	p15, 0, r0, c1, c0, 0
#ifdef CONFIG_CPU_XSCALE
	mrc	p15, 0, r0, c2, c0, 0		/* CPWAIT */
	mov	r0, r0
	sub	pc, pc, #4
#endif

	mov	r0, #0
	mcr	p15, 0, r0, c7, c7, 0		/* invalidate I and D caches */
	mcr	p15, 0, r0, c7, c10, 4		/* drain write buffer */
	mcr	p15, 0, r0, c8, c7, 0		/* invalidate I and D TLBs */
	mcr	p15, 0, r0, c13, c0, 0		/* reset FCSE PID */

#if defined(CONFIG_ARM_ZIMAGE_HEADER)
	/*
	 * zImage loaders do not agree on where the image goes: kexec puts it
	 * at the start of RAM plus 0x8000, QEMU plus 0x10000, U-Boot wherever
	 * bootz is told. Only position independent code may run until the
	 * image sits at its link address, so move it there first.
	 *
	 * The source and the destination usually overlap this very code, so
	 * the copy loop is first moved past the end of both and run from
	 * there. The caches are off, so draining the write buffer is enough
	 * before executing what was copied.
	 */
	adr	r3, __start			/* where we run */
	ldr	r4, =__start			/* where we are linked */
	subs	r5, r3, r4
	beq	_relocated

	ldr	r6, =__rom_region_start		/* r6 = destination */
	add	r7, r6, r5			/* r7 = source */
	ldr	r8, =__end
	sub	r8, r8, r6
	add	r8, r8, #3
	bic	r8, r8, #3			/* r8 = bytes to move */

	/* r9 = first word past both the source and the destination */
	add	r9, r7, r8
	add	r10, r6, r8
	cmp	r9, r10
	movlo	r9, r10

	adr	r10, _mover
	mov	r11, r9
	mov	r12, #(_mover_end - _mover)
1:	ldr	r0, [r10], #4
	str	r0, [r11], #4
	subs	r12, r12, #4
	bne	1b
	mcr	p15, 0, r12, c7, c10, 4		/* drain write buffer */
	mov	pc, r9

	/* Position independent: runs from the copy made above */
_mover:
	cmp	r7, r6
	blo	3f
2:	ldr	r0, [r7], #4			/* moving down: from the bottom */
	str	r0, [r6], #4
	subs	r8, r8, #4
	bne	2b
	b	4f
3:	add	r7, r7, r8			/* moving up: from the top */
	add	r6, r6, r8
5:	ldr	r0, [r7, #-4]!
	str	r0, [r6, #-4]!
	subs	r8, r8, #4
	bne	5b
4:	mcr	p15, 0, r8, c7, c10, 4		/* drain write buffer */
	mov	pc, r4				/* __start at its link address */
_mover_end:

	.ltorg
_relocated:
#endif /* CONFIG_ARM_ZIMAGE_HEADER */

	/*
	 * Install one stack per mode. The exception modes only ever use the
	 * three word scratch area at the top of their stack: the handlers
	 * themselves run in system mode on the interrupted stack.
	 */
	ldr	r0, =(z_arm_fiq_stack + CONFIG_ARMV5_FIQ_STACK_SIZE - EXC_SCRATCH_SIZE)
	msr	cpsr_c, #(MODE_FIQ | I_BIT | F_BIT)
	mov	sp, r0

	ldr	r0, =(z_arm_irq_stack + CONFIG_ARMV5_EXCEPTION_STACK_SIZE - EXC_SCRATCH_SIZE)
	msr	cpsr_c, #(MODE_IRQ | I_BIT | F_BIT)
	mov	sp, r0

	ldr	r0, =(z_arm_abort_stack + CONFIG_ARMV5_EXCEPTION_STACK_SIZE - EXC_SCRATCH_SIZE)
	msr	cpsr_c, #(MODE_ABT | I_BIT | F_BIT)
	mov	sp, r0

	ldr	r0, =(z_arm_undef_stack + CONFIG_ARMV5_EXCEPTION_STACK_SIZE - EXC_SCRATCH_SIZE)
	msr	cpsr_c, #(MODE_UND | I_BIT | F_BIT)
	mov	sp, r0

	ldr	r0, =(z_arm_svc_stack + CONFIG_ARMV5_SVC_STACK_SIZE - EXC_SCRATCH_SIZE)
	msr	cpsr_c, #(MODE_SVC | I_BIT | F_BIT)
	mov	sp, r0

	/* Threads and exception handlers run in system mode. */
	ldr	r0, =(z_arm_sys_stack + CONFIG_ARMV5_SYS_STACK_SIZE)
	msr	cpsr_c, #(MODE_SYS | I_BIT | F_BIT)
	mov	sp, r0

#if defined(CONFIG_SOC_RESET_HOOK)
	bl	soc_reset_hook
#endif

	ldr	r0, =z_prep_c
	bx	r0
