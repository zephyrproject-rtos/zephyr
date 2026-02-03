/*
 * Copyright (c) 2025-2026, Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Exception handlers for ARM9
 */

#include <offsets_short.h>
#include <zephyr/arch/cpu.h>
#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>

#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

GTEXT(z_arm_arm9_exit_exc)
SECTION_SUBSEC_FUNC(TEXT, _HandlerModeExit, z_arm_arm9_exit_exc)
	/* decrement exception depth */
	ldr r2, =_kernel
	ldrb r1, [r2, #_cpu_offset_to_exc_depth]
	sub r1, r1, #1
	strb r1, [r2, #_cpu_offset_to_exc_depth]

	/* Restore r0-r3, r12, lr and return to the current thread. */
	pop {r0-r3, r12, lr}

	ldr r0, [sp, #4]
	msr cpsr_cxsf, r0
	ldr r0, [sp, #-24]

	add sp, sp, #8
	ldr pc, [sp, #-8]
