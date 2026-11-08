/*
 * Copyright (c) 2026 Takayuki Goto
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Thread context switching for ARMv5 (ARM9/XScale)
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <zephyr/arch/cpu.h>
#include <offsets_short.h>
#include <zephyr/kernel.h>
#include "macro_priv.inc"

_ASM_FILE_PROLOGUE

GTEXT(z_arm_context_switch)

/*
 * void z_arm_context_switch(struct k_thread *new, struct k_thread *old);
 *
 * Called either from arch_switch() (cooperative) or from z_armv5_irq_done()
 * (preemptive). Runs in system mode, so the sp/lr saved below are r13_sys and
 * r14_sys, i.e. the registers threads actually run on.
 */
SECTION_FUNC(TEXT, z_arm_context_switch)

	ldr	r2, =_thread_offset_to_callee_saved
	add	r2, r1, r2
	stm	r2, {r4-r11, sp, lr}

	/* hand the exception depth over to the incoming thread */
	get_cpu	r2
	ldrb	r3, [r2, #_cpu_offset_to_exc_depth]
	strb	r3, [r1, #_thread_offset_to_exception_depth]
	ldrb	r3, [r0, #_thread_offset_to_exception_depth]
	strb	r3, [r2, #_cpu_offset_to_exc_depth]

	/*
	 * Publish the outgoing thread as its own switch handle. This must
	 * happen only once all of its state has been saved; ARMv5 has no DSB,
	 * so drain the write buffer instead.
	 */
	mov	r3, #0
	mcr	p15, 0, r3, c7, c10, 4
	str	r1, [r1, #___thread_t_switch_handle_OFFSET]

	ldr	r2, =_thread_offset_to_callee_saved
	add	r2, r0, r2
	ldm	r2, {r4-r11, sp, lr}

#ifdef CONFIG_INSTRUMENT_THREAD_SWITCHING
	stmdb	sp!, {r0, lr}
	bl	z_thread_mark_switched_in
	ldmia	sp!, {r0, lr}
#endif

	bx	lr
