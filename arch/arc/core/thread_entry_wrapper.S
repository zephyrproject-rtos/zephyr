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
