/*
 * Copyright (c) 2025 NVIDIA Corporation <jholdsworth@nvidia.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <offsets_short.h>

#include "asm_macros.inc"

#define THREAD_O(FIELD)	_thread_offset_to_##FIELD

.macro do_callee_saved op, reg
	\op r2, THREAD_O(r2), \reg
	\op r9, THREAD_O(r9), \reg
	\op r10, THREAD_O(r10), \reg
	\op r14, THREAD_O(r14), \reg
	\op r16, THREAD_O(r16), \reg
	\op r18, THREAD_O(r18), \reg
	\op r20, THREAD_O(r20), \reg
	\op r22, THREAD_O(r22), \reg
	\op r24, THREAD_O(r24), \reg
	\op r26, THREAD_O(r26), \reg
	\op r28, THREAD_O(r28), \reg
	\op r30, THREAD_O(r30), \reg
.endm

.macro store_callee_saved reg
	do_callee_saved op_store_reg, \reg
.endm

.macro load_callee_saved reg
	do_callee_saved op_load_reg, \reg
.endm

GTEXT(z_openrisc_switch)

/* void z_openrisc_switch(k_thread_t *switch_to, k_thread_t *switch_from) */
SECTION_FUNC(TEXT, z_openrisc_switch)

	/* Save the old thread's callee-saved registers */
	store_callee_saved r4

	/* Save the old thread's stack pointer */
	l.sw _thread_offset_to_r1(r4), r1

	/* Set thread->switch_handle = thread to mark completion */
	l.sw ___thread_t_switch_handle_OFFSET(r4), r4

	/* Get the new thread's stack pointer */
	l.lwz r1, _thread_offset_to_r1(r3)

#if CONFIG_INSTRUMENT_THREAD_SWITCHING
	/* Save r3 to r14, then restore it after use */
#ifdef __OR1K_NODELAY__
	l.ori r14, r3, 0
	l.jal z_thread_mark_switched_in
#else
	l.jal z_thread_mark_switched_in
	 l.ori r14, r3, 0
#endif
	l.ori r3, r14, 0
#endif

	/* Restore the new thread's callee-saved registers */
	load_callee_saved r3

	/* Return to arch_switch() or _irq_wrapper() */
	l.jr r9
#ifndef __OR1K_NODELAY__
	 l.nop
#endif
