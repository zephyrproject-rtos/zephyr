/*
 * Copyright (c) 2024 Infineon Technologies AG
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/sys/util.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>

GTEXT(__start)
GTEXT(z_prep_c)
GTEXT(z_tricore_wdt_boot)

GDATA(z_interrupt_stacks)
GDATA(z_triore_csa)
GDATA(__biv)
GDATA(__btv)

GTEXT(aurix_cpu_endinit_disable)
GTEXT(aurix_cpu_endinit_enable)
GTEXT(aurix_safety_endinit_enable)

#define INITIAL_PSW 0x00000B80

#if CONFIG_TRICORE_RESET_TRAMPOLINE
SECTION_FUNC(reset,__start)
	movh.a	%a15, hi:__start_stage2
	lea	%a15, [%a15], lo:__start_stage2
	ji	%a15

SECTION_FUNC(TEXT,__start_stage2)
#else
SECTION_FUNC(reset,__start)
#endif
	/* Disable endinit protection, in case of reboot */
	jl tricore_cpu_endinit_disable

	/* Set base trap vector table pointer */
	mov	%d10, lo:__btv
	addih	%d10, %d10, hi:__btv
	mtcr	0xFE24, %d10
	isync

	/* Initialize context save areas */
	movh.a	%a15, hi:z_tricore_csa
	lea	%a15, [%a15], lo:z_tricore_csa
	mov	%d15, lo:(CONFIG_TRICORE_CSA_COUNT*64)
	addsc.a	%a14, %a15, %d15, 0

	/* Setup FCX register */
	mov.d	%d15, %a15
	extr.u	%d14, %d15, 6, 16
	extr.u	%d13, %d15, 12, 20
	insert	%d15, %d13, %d14, 0, 16
	mtcr	0xFE38, %d15
	isync

#if IS_ENABLED(CONFIG_CPU_TC18) && IS_ENABLED(CONFIG_CPU_HAS_TRICORE_VIRTUALIZATION) && \
	!IS_ENABLED(CONFIG_TRICORE_VIRTUALIZATION)
	/* Set initial PSW for virtualization mode */
	mov	%d14, lo:(INITIAL_PSW)
	addih	%d14, %d14, hi:(INITIAL_PSW)
	st.w	[%a15]4, %d14
	dsync
	/* Disable virtualization if not required. Call returns via rfh instruction */
	call	tricore_cpu_disable_virtualization
	/* Re-setup BTV register after disabling virtualization */
	mtcr	0xFE24, %d10
	/* Re-setup FCX register after disabling virtualization */
	mtcr	0xFE38, %d15
#endif

csa_loop:
	addi	%d15, %d15, 1
	st.w	[%a15], %d15
	lea	%a15, [%a15], 64
	jne.a	%a15, %a14, csa_loop

	/* Null-terminate last CSA in free list */
	mov	%d14, 0
	st.w	[%a14]-64, %d14

	/* Store LCX with 3 less entries for context depletion trap */
	addi	%d15, %d15, -3
	mtcr	0xFE3C, %d15
	isync

	/* Set base interrupt vector table pointer */
	mov	%d15, lo:__biv
	addih	%d15, %d15, hi:__biv
	mtcr	0xFE20, %d15

	/* Set interrupt stack pointer */
	mov	%d15, lo:(z_interrupt_stacks + CONFIG_ISR_STACK_SIZE)
	addih	%d15, %d15, hi:(z_interrupt_stacks + CONFIG_ISR_STACK_SIZE)
	mtcr	0xFE28, %d15

	/* Set initial program status word */
	mov	%d15, lo:(INITIAL_PSW)
	addih	%d15, %d15, hi:(INITIAL_PSW)
	mtcr	0xFE04, %d15

	/* Ensure all changes to control registers take effect */
	isync

	/* Lock cpu and safety endinit again */
	jl tricore_cpu_endinit_enable
	jl tricore_safety_endinit_enable

	/* Set main stack pointer */
	movh.a	%a10, hi:(z_interrupt_stacks + CONFIG_ISR_STACK_SIZE)
	lea	%a10, [%a10], lo:(z_interrupt_stacks + CONFIG_ISR_STACK_SIZE)

	/* Watchdog boot procedure */
	call z_tricore_wdt_boot

	/* Call init code */
	call z_prep_c
