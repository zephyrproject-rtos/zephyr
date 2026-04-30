/*
 * Copyright (c) 2025-2026, Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread context switching for ARM9
 *
 * This module implements the routines necessary for thread context switching
 * on ARM9 CPUs.
 */

#include "macro_priv.inc"
#include <offsets_short.h>
#include <zephyr/linker/sections.h>

_ASM_FILE_PROLOGUE

GTEXT(z_arm_context_switch)

/*
 * Routine to handle context switches
 * void z_arm_context_switch(struct k_thread *new, struct k_thread *old);
 */
SECTION_FUNC(TEXT, z_arm_context_switch)
	ldr r2, =_thread_offset_to_callee_saved
	add r2, r1, r2
	stm r2, {r4-r11, sp, lr}

	/* save current thread's exception depth */
	ldr r2, =_kernel
	ldrb r3, [r2, #_cpu_offset_to_exc_depth]
	strb r3, [r1, #_thread_offset_to_exception_depth]

	/* retrieve next thread's exception depth */
	ldrb r3, [r0, #_thread_offset_to_exception_depth]
	strb r3, [r2, #_cpu_offset_to_exc_depth]

	z_arm_arm9_drain_write_buffer r2

	str r1, [r1, #___thread_t_switch_handle_OFFSET]

	ldr r2, =_thread_offset_to_callee_saved
	add r2, r0, r2
	ldm r2, {r4-r11, sp, lr}

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
	push {lr}
	bl z_thread_mark_switched_in
	pop {lr}
#endif

	bx lr
