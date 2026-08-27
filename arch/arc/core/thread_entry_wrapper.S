/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Wrapper for z_thread_entry
 *
 * Wrapper for z_thread_entry routine when called from the initial context.
 */

#include <zephyr/toolchain.h>
#include <zephyr/linker/sections.h>
#include <v2/irq.h>
#include <zephyr/arch/arc/asm-compat/assembler.h>

GTEXT(z_thread_entry_wrapper)
GTEXT(z_thread_entry_wrapper1)

/**
 * @brief Wrapper for z_thread_entry
 *
 * The routine pops parameters for the z_thread_entry from stack frame, prepared
 * by the arch_new_thread() routine.
 */

SECTION_FUNC(TEXT, z_thread_entry_wrapper)
	seti _ARC_V2_INIT_IRQ_LOCK_KEY
z_thread_entry_wrapper1:
	POPR r3
	POPR r2
	POPR r1
	POPR r0
	j z_thread_entry
	nop


#if !defined(CONFIG_MULTITHREADING)
GTEXT(z_main_no_multithreading_entry_wrapper)

/* void z_main_no_multithreading_entry_wrapper(*p1, *p2, *p3, *main_stack, *main_entry) */
SECTION_FUNC(TEXT, z_main_no_multithreading_entry_wrapper)
	MOVR sp, r3

	/* *p1, *p2, *p3 are in r0, r1, r2 already */
	jl [r4]
	nop

	/*
	 * If we return from main we'll return from main wrapper and appear here.
	 * Go to infinite loop as there is nothing more to do.
	 */
z_main_no_multithreading_entry_wrapper_end:
	b z_main_no_multithreading_entry_wrapper_end

#endif /* !CONFIG_MULTITHREADING */
