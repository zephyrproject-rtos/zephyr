/*
 * Copyright 2026 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Cortex-M SysTick wrapper for perf stack sampling
 *
 * The normal SysTick handler is a C function, so its compiler-generated
 * prologue may change the handler stack before perf can inspect the
 * exception-entry state. This wrapper runs directly from the SysTick vector
 * and captures the interrupted stack pointer, EXC_RETURN, and frame pointer
 * before calling sys_clock_isr().
 */

#include <zephyr/toolchain.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(z_arm_perf_sys_clock_isr)
GTEXT(sys_clock_isr)
GTEXT(z_arm_perf_sample_enter)
GTEXT(z_arm_perf_sample_exit)

SECTION_FUNC(TEXT, z_arm_perf_sys_clock_isr)
	/* Use EXC_RETURN to find the hardware exception frame before
	 * modifying the handler stack. The C timer ISR runs after exception
	 * entry, so the wrapper must capture the interrupted stack pointer,
	 * EXC_RETURN, and frame pointer before calling into sys_clock_isr().
	 */
#if defined(CONFIG_ARMV6_M_ARMV8_M_BASELINE)
	movs r0, #_EXC_RETURN_SPSEL_Msk
	mov r1, lr
	tst r1, r0
	beq .L_stack_frame_msp
	mrs r0, PSP
	b .L_stack_frame_endif
.L_stack_frame_msp:
	mrs r0, MSP
.L_stack_frame_endif:
#elif defined(CONFIG_ARMV7_M_ARMV8_M_MAINLINE)
	tst lr, #_EXC_RETURN_SPSEL_Msk
	ite eq
	mrseq r0, MSP
	mrsne r0, PSP
#else
#error Unknown ARM architecture
#endif

	mov r1, lr
	mov r2, r7
	/* Keep the original EXC_RETURN live across the profiling hooks and
	 * the real SysTick handler so the exception returns exactly as it
	 * would without perf sampling enabled.
	 */
	push {r3, lr}
	bl z_arm_perf_sample_enter
	bl sys_clock_isr
	bl z_arm_perf_sample_exit
	pop {r2, r3}
	mov lr, r3
	bx lr
